from pathlib import Path

import apsw
from apsw import Connection as SQLite

APP_NAME = "notf_studio"
APP_VERSION = "v0.1.0"
DB_SUFFIX = '.sqlite'


def create_app_db(connection: SQLite) -> None:
    pass


def open_app_db() -> SQLite:
    app_directory: Path = Path.home() / Path(".config") / Path(APP_NAME) / Path(APP_VERSION)
    app_directory.mkdir(parents=True, exist_ok=True)
    db_path: Path = app_directory / f'{APP_NAME}{DB_SUFFIX}'
    # if not db_path.exists():
    #

    return SQLite(db_path.as_posix())


def myupdatehook(type, databasename, tablename, rowid):
    print("Updated: %s database %s, table %s, row %d" % (
        apsw.mapping_authorizer_function[type], databasename, tablename, rowid))


connection: SQLite = open_app_db()
cursor = connection.cursor()

cursor.execute("create table s(str)")
connection.setupdatehook(myupdatehook)
cursor.execute("insert into s values(?)", ("file93",))
cursor.execute("update s set str=? where str=?", ("file94", "file93"))
cursor.execute("delete from s where str=?", ("file94",))
connection.setupdatehook(None)
