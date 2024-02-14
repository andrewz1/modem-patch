#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "sqlite3.h"
#include "xlist.h"
#include "xlog.h"

#define TARGET_ID "it_iliad"
#define ID_F (1 << 0)
#define HASH_F (1 << 1)

#define RU_MCC 250
#define UA_MCC 255
#define BY_MCC 257
#define KZ_MCC 401

typedef struct mccmnc {
	list_head list;
	char id[16];
	char name[256];
} mccmnc;

static const char tgtReq[] = "SELECT DISTINCT confnames.carrier_id, confmap.confman "
							 "FROM confnames, confmap "
							 "WHERE confnames.name = '%s' "
							 "AND confmap.carrier_id = confnames.carrier_id";

static const char mccReq[] = "SELECT DISTINCT carrier_info.carrier_id, carrier_name.name "
							 "FROM carrier_info, carrier_name "
							 "WHERE carrier_info.mccmnc LIKE '%d%%' "
							 "AND carrier_info.carrier_id = carrier_name.carrier_id";

static const char updReq[] = "INSERT OR IGNORE INTO confmap VALUES('%s', NULL, '%s')";
static const char upd0Req[] = "UPDATE confmap SET confman = '%s' WHERE carrier_id = '0'";

static char tgtId[16];
static char tgtHash[64];

static LIST_HEAD(mccList);

#define dbClose(x)             \
	do {                       \
		if (x) {               \
			sqlite3_close(db); \
			(x) = NULL;        \
		}                      \
	} while (0)

static sqlite3 *dbOpen(const char *name)
{
	sqlite3 *db;
	if (sqlite3_open(name, &db) != SQLITE_OK) {
		errorf("dbOpen: %s\n", sqlite3_errmsg(db));
		dbClose(db);
	}
	return db;
}

static int tgtCb(void *data, int argc, char **argv, char **azColName)
{
	if (argc != 2)
		return -1;
	int *cnt = data;
	if (!tgtId[0]) {
		strncpy(tgtId, argv[0], sizeof(tgtId) - 1);
		(*cnt) |= ID_F;
	}
	if (!tgtHash[0]) {
		strncpy(tgtHash, argv[1], sizeof(tgtHash) - 1);
		(*cnt) |= HASH_F;
	}
	return 0;
}

static int getTgt(sqlite3 *db)
{
	char sql[512];
	sprintf(sql, tgtReq, TARGET_ID);
	int cnt = 0;
	if (sqlite3_exec(db, sql, tgtCb, &cnt, NULL) != SQLITE_OK) {
		errorf("getTgt: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	if (cnt != (ID_F | HASH_F)) {
		errorf("id or hash not found for %s\n", TARGET_ID);
	}
	return 0;
}

static int mccCb(void *data, int argc, char **argv, char **azColName)
{
	if (argc != 2)
		return -1;
	mccmnc *v;
	list_for_each_entry(v, &mccList, list)
		if (strcmp(argv[0], v->id) == 0)
			return 0;
	mccmnc *m = calloc(1, sizeof(*m));
	INIT_LIST_HEAD(&m->list);
	strncpy(m->id, argv[0], sizeof(m->id) - 1);
	strncpy(m->name, argv[1], sizeof(m->name) - 1);
	list_add_tail(&m->list, &mccList);
	return 0;
}

static int getMcc(sqlite3 *db, int mcc)
{
	char sql[512];
	sprintf(sql, mccReq, mcc);
	if (sqlite3_exec(db, sql, mccCb, NULL, NULL) != SQLITE_OK) {
		errorf("getMcc(%d): %s\n", mcc, sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}

static int putMcc(sqlite3 *db, const char *id)
{
	char sql[512];
	sprintf(sql, updReq, id, tgtHash);
	if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK) {
		errorf("putMcc(%s): %s\n", id, sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}

static int upd0Mcc(sqlite3 *db)
{
	char sql[512];
	sprintf(sql, upd0Req, tgtHash);
	if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK) {
		errorf("putMcc(0): %s\n", sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		exitf("usage: %s db_file\n", argv[0]);
	if (access(argv[1], R_OK | W_OK))
		exitf("file %s: %m\n", argv[1]);
	int rv = EXIT_FAILURE;
	sqlite3 *db = dbOpen(argv[1]);
	if (!db)
		return rv;
	if (getTgt(db))
		goto out;
	infof("target name '%s', id %s, hash '%s'\n", TARGET_ID, tgtId, tgtHash);
	if (getMcc(db, RU_MCC))
		goto out;
	if (getMcc(db, UA_MCC))
		goto out;
	if (getMcc(db, BY_MCC))
		goto out;
	if (getMcc(db, KZ_MCC))
		goto out;
	mccmnc *v;
	list_for_each_entry(v, &mccList, list) {
		infof("update record id %s, operator '%s'\n", v->id, v->name);
		if (putMcc(db, v->id))
			goto out;
	}
	infof("update default record id 0\n");
	if (upd0Mcc(db))
		goto out;
	rv = EXIT_SUCCESS;
out:
	dbClose(db);
	mccmnc *n;
	list_for_each_entry_safe(v, n, &mccList, list) {
		list_del_init(&v->list);
		free(v);
	}
	return rv;
}
