/*  Copyright (C) 2014 CZ.NIC, z.s.p.o. <knot-dns@labs.nic.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_LMDB

#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <lmdb.h>

#include "libknot/internal/namedb/namedb_lmdb.h"
#include "libknot/errcode.h"

#define LMDB_DIR_MODE	0770
#define LMDB_FILE_MODE	0660
#define LMDB_MAPSIZE	(100 * 1024 * 1024)

struct lmdb_env
{
	MDB_dbi dbi;
	MDB_env *env;
	mm_ctx_t *pool;
};

static int create_env_dir(const char *path)
{
	int r = mkdir(path, LMDB_DIR_MODE);
	if (r == -1 && errno != EEXIST) {
		return knot_errno_to_error(errno);
	}

	return KNOT_EOK;
}

/*!
 * \brief Convert error code returned by LMDB to Knot DNS error code.
 *
 * LMDB defines own error codes but uses additional ones from libc. All LMDB
 * specific error codes are translated to KNOT_DATABASE_ERROR.
 */
static int lmdb_error_to_knot(int error)
{
	if (error == MDB_SUCCESS) {
		return KNOT_EOK;
	}

	if (MDB_KEYEXIST <= error && error <= MDB_LAST_ERRCODE) {
		return KNOT_DATABASE_ERROR;
	}

	return knot_errno_to_error(error);
}

static int dbase_open(struct lmdb_env *env, const char *path)
{
	int ret = mdb_env_create(&env->env);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0) {
		mdb_env_close(env->env);
		return KNOT_EINVAL;
	}

	size_t map_size = (LMDB_MAPSIZE / page_size) * page_size;
	ret = mdb_env_set_mapsize(env->env, map_size);
	if (ret != MDB_SUCCESS) {
		mdb_env_close(env->env);
		return lmdb_error_to_knot(ret);
	}

	ret = create_env_dir(path);
	if (ret != KNOT_EOK) {
		mdb_env_close(env->env);
		return ret;
	}

	ret = mdb_env_open(env->env, path, 0, LMDB_FILE_MODE);
	if (ret != MDB_SUCCESS) {
		mdb_env_close(env->env);
		return lmdb_error_to_knot(ret);
	}

	MDB_txn *txn = NULL;
	ret = mdb_txn_begin(env->env, NULL, 0, &txn);
	if (ret != MDB_SUCCESS) {
		mdb_env_close(env->env);
		return lmdb_error_to_knot(ret);
	}

	ret = mdb_open(txn, NULL, 0, &env->dbi);
	if (ret != MDB_SUCCESS) {
		mdb_txn_abort(txn);
		mdb_env_close(env->env);
		return lmdb_error_to_knot(ret);
	}

	ret = mdb_txn_commit(txn);
	if (ret != MDB_SUCCESS) {
		mdb_env_close(env->env);
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}

static void dbase_close(struct lmdb_env *env)
{
	mdb_close(env->env, env->dbi);
	mdb_env_close(env->env);
}

static int init(const char *config, namedb_t **db_ptr, mm_ctx_t *mm)
{
	struct lmdb_env *env = mm_alloc(mm, sizeof(struct lmdb_env));
	if (env == NULL) {
		return KNOT_ENOMEM;
	}
	memset(env, 0, sizeof(struct lmdb_env));

	int ret = dbase_open(env, config);
	if (ret != KNOT_EOK) {
		mm_free(mm, env);
		return ret;
	}

	env->pool = mm;
	*db_ptr = env;

	return KNOT_EOK;
}

static void deinit(namedb_t *db)
{
	if (db) {
		struct lmdb_env *env = db;

		dbase_close(env);
		mm_free(env->pool, env);
	}
}

static int txn_begin(namedb_t *db, namedb_txn_t *txn, unsigned flags)
{
	txn->db = db;
	txn->txn = NULL;

	unsigned txn_flags = 0;
	if (flags & NAMEDB_RDONLY) {
		txn_flags |= MDB_RDONLY;
	}

	struct lmdb_env *env = db;
	int ret = mdb_txn_begin(env->env, NULL, txn_flags, (MDB_txn **)&txn->txn);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}

static int txn_commit(namedb_txn_t *txn)
{
	int ret = mdb_txn_commit((MDB_txn *)txn->txn);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}

static void txn_abort(namedb_txn_t *txn)
{
	mdb_txn_abort((MDB_txn *)txn->txn);
}

static int count(namedb_txn_t *txn)
{
	struct lmdb_env *env = txn->db;

	MDB_stat stat;
	int ret = mdb_stat(txn->txn, env->dbi, &stat);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return stat.ms_entries;
}

static int clear(namedb_txn_t *txn)
{
	struct lmdb_env *env = txn->db;

	int ret = mdb_drop(txn->txn, env->dbi, 0);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}

static namedb_iter_t *iter_set(namedb_iter_t *iter, namedb_val_t *key, unsigned flags)
{
	MDB_cursor *cursor = iter;

	MDB_cursor_op op = MDB_SET;
	switch(flags) {
	case NAMEDB_NOOP:  return cursor;
	case NAMEDB_FIRST: op = MDB_FIRST; break;
	case NAMEDB_LAST:  op = MDB_LAST;  break;
	case NAMEDB_NEXT:  op = MDB_NEXT; break;
	case NAMEDB_PREV:  op = MDB_PREV; break;
	case NAMEDB_LEQ:
	case NAMEDB_GEQ:   op = MDB_SET_RANGE; break;
	default: break;
	}

	MDB_val db_key = { 0, NULL };
	if (key) {
		db_key.mv_data = key->data;
		db_key.mv_size = key->len;
	}

	int ret = mdb_cursor_get(cursor, key ? &db_key : NULL, NULL, op);

	/* LEQ is not supported in LMDB, workaround using GEQ. */
	if (flags == NAMEDB_LEQ && key) {
		/* Searched key is after the last key. */
		if (ret != MDB_SUCCESS) {
			return iter_set(iter, NULL, NAMEDB_LAST);
		}
		/* If the searched key != matched, get previous. */
		if ((key->len != db_key.mv_size) || (memcmp(key->data, db_key.mv_data, key->len) != 0)) {
			return iter_set(iter, NULL, NAMEDB_PREV);
		}
	}

	if (ret != MDB_SUCCESS) {
		mdb_cursor_close(cursor);
		return NULL;
	}

	return cursor;
}

static namedb_iter_t *iter_begin(namedb_txn_t *txn, unsigned flags)
{
	struct lmdb_env *env = txn->db;
	MDB_cursor *cursor = NULL;

	int ret = mdb_cursor_open(txn->txn, env->dbi, &cursor);
	if (ret != MDB_SUCCESS) {
		return NULL;
	}

	/* Clear sorted flag, as it's always sorted. */
	flags &= ~NAMEDB_SORTED;

	return iter_set(cursor, NULL, (flags == 0) ? NAMEDB_FIRST : flags);
}

static namedb_iter_t *iter_next(namedb_iter_t *iter)
{
	return iter_set(iter, NULL, NAMEDB_NEXT);
}

static int iter_key(namedb_iter_t *iter, namedb_val_t *key)
{
	MDB_cursor *cursor = iter;

	MDB_val mdb_key, mdb_val;
	int ret = mdb_cursor_get(cursor, &mdb_key, &mdb_val, MDB_GET_CURRENT);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	key->data = mdb_key.mv_data;
	key->len  = mdb_key.mv_size;
	return KNOT_EOK;
}

static int iter_val(namedb_iter_t *iter, namedb_val_t *val)
{
	MDB_cursor *cursor = iter;

	MDB_val mdb_key, mdb_val;
	int ret = mdb_cursor_get(cursor, &mdb_key, &mdb_val, MDB_GET_CURRENT);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	val->data = mdb_val.mv_data;
	val->len  = mdb_val.mv_size;
	return KNOT_EOK;
}

static void iter_finish(namedb_iter_t *iter)
{
	if (iter == NULL) {
		return;
	}

	MDB_cursor *cursor = iter;
	mdb_cursor_close(cursor);
}

static int find(namedb_txn_t *txn, namedb_val_t *key, namedb_val_t *val, unsigned flags)
{
	namedb_iter_t *iter = iter_begin(txn, NAMEDB_NOOP);
	if (iter == NULL) {
		return KNOT_ERROR;
	}

	int ret = KNOT_EOK;
	if (iter_set(iter, key, flags) == NULL) {
		return KNOT_ENOENT;
	} else {
		ret = iter_val(iter, val);
	}

	iter_finish(iter);
	return ret;
}

static int insert(namedb_txn_t *txn, namedb_val_t *key, namedb_val_t *val, unsigned flags)
{
	struct lmdb_env *env = txn->db;

	MDB_val db_key = { key->len, key->data };
	MDB_val data = { val->len, val->data };

	int ret = mdb_put(txn->txn, env->dbi, &db_key, &data, 0);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}

static int del(namedb_txn_t *txn, namedb_val_t *key)
{
	struct lmdb_env *env = txn->db;
	MDB_val db_key = { key->len, key->data };
	MDB_val data = { 0, NULL };

	int ret = mdb_del(txn->txn, env->dbi, &db_key, &data);
	if (ret != MDB_SUCCESS) {
		return lmdb_error_to_knot(ret);
	}

	return KNOT_EOK;
}


const struct namedb_api *namedb_lmdb_api(void)
{
	static const struct namedb_api api = {
		"lmdb",
		init, deinit,
		txn_begin, txn_commit, txn_abort,
		count, clear, find, insert, del,
		iter_begin, iter_set, iter_next, iter_key, iter_val, iter_finish
	};

	return &api;
}

#else

#include <stdlib.h>

const namedb_api_t *namedb_lmdb_api(void)
{
	return NULL;
}

#endif
