"""Rebuild the compact desktop runtime catalogue from a canonical ProGen3D database."""
import argparse
import hashlib
from pathlib import Path
import sqlite3
import tempfile


def synchronize(source: Path, destination: Path) -> None:
    if source.resolve() == destination.resolve():
        raise ValueError("Select a destination different from the source database.")
    destination.parent.mkdir(parents=True, exist_ok=True)
    source_db = sqlite3.connect(source.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        source_db.execute("BEGIN")
        with tempfile.TemporaryDirectory(dir=destination.parent) as directory:
            staged = Path(directory) / destination.name
            db = sqlite3.connect(staged)
            try:
                db.execute("PRAGMA foreign_keys=ON")
                db.executescript("""
                    CREATE TABLE catalogue(ref TEXT PRIMARY KEY,title TEXT NOT NULL,category TEXT NOT NULL,search_text TEXT NOT NULL);
                    CREATE TABLE standalone(ref TEXT PRIMARY KEY REFERENCES catalogue,grammar TEXT NOT NULL,sha256 TEXT NOT NULL);
                    CREATE TABLE parameters(ref TEXT REFERENCES catalogue,ordinal INTEGER,name TEXT,unit TEXT,default_value REAL,minimum REAL,maximum REAL,PRIMARY KEY(ref,name));
                    CREATE TABLE properties(ref TEXT REFERENCES catalogue,name TEXT,value,PRIMARY KEY(ref,name));
                    CREATE TABLE notes(topic TEXT PRIMARY KEY,description TEXT);
                """)
                queries = {
                    "catalogue": "SELECT ref,title,category,search_text FROM catalogue ORDER BY ref",
                    "standalone": "SELECT ref,grammar,sha256 FROM standalone ORDER BY ref",
                    "parameters": "SELECT ref,ordinal,name,unit,default_value,minimum,maximum FROM parameters ORDER BY ref,ordinal",
                    "properties": "SELECT ref,name,value FROM properties WHERE name LIKE 'integer:%' ORDER BY ref,name",
                    "notes": "SELECT topic,description FROM notes WHERE topic='revision'",
                }
                for table, query in queries.items():
                    cursor = source_db.execute(query)
                    placeholders = ",".join("?" for _ in cursor.description)
                    db.executemany(f"INSERT INTO {table} VALUES ({placeholders})", cursor)
                if db.execute("SELECT ref FROM catalogue EXCEPT SELECT ref FROM standalone").fetchone():
                    raise ValueError("A catalogue entry lacks its standalone grammar")
                for ref, grammar, checksum in db.execute("SELECT * FROM standalone"):
                    if hashlib.sha256(grammar.encode()).hexdigest() != checksum:
                        raise ValueError("Source checksum mismatch for " + ref)
                db.commit()
                if db.execute("PRAGMA integrity_check").fetchone() != ("ok",):
                    raise ValueError("Runtime catalogue failed integrity check")
            finally:
                db.close()
            staged.replace(destination)
    finally:
        source_db.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path, nargs="?",
                        default=Path(__file__).resolve().parents[1] / "resources/reuse/catalogue.sqlite3")
    args = parser.parse_args()
    synchronize(args.source, args.destination)
