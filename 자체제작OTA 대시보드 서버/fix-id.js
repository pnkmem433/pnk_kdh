const sqlite3 = require('sqlite3').verbose();
const path = require('path');
const dbPath = path.join(__dirname, 'local-ota.sqlite');
const db = new sqlite3.Database(dbPath);

db.serialize(() => {
    db.run("UPDATE project SET id = 10 WHERE id = 1", function(err) {
        if (err) { console.error("오류:", err.message); }
        else { console.log("ID 변경 완료: " + this.changes + "개"); }
        db.close();
    });
});