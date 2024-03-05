#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "sqlite3.h"
#include "xlist.h"
#include "xlog.h"

#define TARGET_ID "it_iliad"
//#define TARGET_ID "testsim"

#define ID_F (1 << 0)
#define HASH_F (1 << 1)

typedef struct mccmnc {
	list_head list;
	char id[16];
	char mccmnc[16];
	char name[256];
} mccmnc;

static const char tgtReq[] = "SELECT DISTINCT confnames.carrier_id, confmap.confman "
							 "FROM confnames, confmap "
							 "WHERE confnames.name = '%s' "
							 "AND confmap.carrier_id = confnames.carrier_id";

static const char mccReq[] = "SELECT DISTINCT carrier_info.carrier_id, carrier_name.name, carrier_info.mccmnc "
							 "FROM carrier_info, carrier_name "
							 "WHERE substr(carrier_info.mccmnc, 1, 3) = '%s'"
							 "AND carrier_info.carrier_id = carrier_name.carrier_id";

static const char updReq[] = "INSERT OR REPLACE INTO confmap VALUES('%s', NULL, '%s')";

static const char updDefMcc[] = "INSERT OR REPLACE INTO regional_fallback VALUES('%s', '%s')";

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
	if (argc != 3)
		return -1;
	mccmnc *v;
	list_for_each_entry(v, &mccList, list)
		if (strcmp(argv[0], v->id) == 0)
			return 0;
	mccmnc *m = malloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	INIT_LIST_HEAD(&m->list);
	strncpy(m->id, argv[0], sizeof(m->id) - 1);
	strncpy(m->name, argv[1], sizeof(m->name) - 1);
	strncpy(m->mccmnc, argv[2], sizeof(m->mccmnc) - 1);
	list_add_tail(&m->list, &mccList);
	return 0;
}

static int getMcc(sqlite3 *db, const char *mcc)
{
	char sql[512];
	sprintf(sql, mccReq, mcc);
	if (sqlite3_exec(db, sql, mccCb, NULL, NULL) != SQLITE_OK) {
		errorf("getMcc(%s): %s\n", mcc, sqlite3_errmsg(db));
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

static int putDefMcc(sqlite3 *db, const char *id)
{
	char sql[512];
	sprintf(sql, updDefMcc, id, tgtId);
	if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK) {
		errorf("putDefMcc(%s): %s\n", id, sqlite3_errmsg(db));
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2)
		exitf("usage: %s db_file [mcc list]\n", argv[0]);
	if (access(argv[1], R_OK | W_OK))
		exitf("file %s: %m\n", argv[1]);
	if (argc == 2)
		return 0;
	int rv = EXIT_FAILURE;
	sqlite3 *db = dbOpen(argv[1]);
	if (!db)
		return rv;
	if (getTgt(db))
		goto out;
	infof("target name '%s', id %s, hash '%s'\n", TARGET_ID, tgtId, tgtHash);
	int i;
	for (i = 2; i < argc; i++) {
		if (getMcc(db, argv[i]))
			break;
		if (putDefMcc(db, argv[i]))
			break;
	}
	if (i < argc)
		goto out;
	mccmnc *v;
	list_for_each_entry(v, &mccList, list) {
		infof("update record id %s, mccmnc '%s', operator '%s'\n", v->id, v->mccmnc, v->name);
		if (putMcc(db, v->id))
			goto out;
	}
	infof("update default record id 0\n");
	if (putMcc(db, "0"))
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
