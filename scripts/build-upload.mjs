import { execSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

const fqbn = process.argv[2] || "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashMode=qio120,PartitionScheme=min_spiffs,LoopCore=0,EventsCore=0";
const port = process.argv[3] || "COM3";
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const defaultSketch = path.resolve(__dirname, "..");
const sketch = process.argv[4] || defaultSketch;
const jobs = process.argv[5] || "0";
const sketchWin = sketch.replace(/\//g, "\\");

const cmd = `powershell -ExecutionPolicy Bypass -File "${sketchWin}\\scripts\\build-and-upload.ps1" -Fqbn "${fqbn}" -Port "${port}" -Sketch "${sketchWin}" -Jobs ${jobs}`;
console.log("[node]", cmd);
execSync(cmd, { stdio: "inherit" });
