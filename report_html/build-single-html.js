const fs = require("fs");
const path = require("path");

const root = __dirname;
const indexPath = path.join(root, "index.html");
const dataPath = path.join(root, "data.js");
const outputPath = path.join(root, "report_single.html");

const indexHtml = fs.readFileSync(indexPath, "utf8");
const dataJs = fs.readFileSync(dataPath, "utf8");

const inlinedData = dataJs.replace(/\bexport const\b/g, "const");

const importBlock = /import\s*\{[\s\S]*?\}\s*from\s*"\.\/data\.js";\s*/;

if (!importBlock.test(indexHtml)) {
  throw new Error("Could not find data.js import block in index.html");
}

const mergedHtml = indexHtml.replace(importBlock, `${inlinedData}\n\n`);

fs.writeFileSync(outputPath, mergedHtml, "utf8");

console.log(`Created: ${outputPath}`);
