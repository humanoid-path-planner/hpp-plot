import { useState } from "react";
import { downloadGraphPng, downloadGraphJson } from "../utils/downloadGraph";


export default function DownloadForm({ close, cyRef }) {

    const [filename, setFilename] = useState("graph");
    const [format, setFormat] = useState("png");
    const [background, setBackground] = useState("transparent");

    const handleDownload = () => {
        console.log("Download requested with filename:", filename, "format:", format, "background:", background);
        const cy = cyRef?.current;

        if (format === "png") {
            console.log("Downloading PNG with background:", background);
        downloadGraphPng(cy, filename, background);
        } else if (format === "json") {
            console.log("Downloading JSON");
        downloadGraphJson(cy, filename);
        }
        close();
    };


  return (
    <div id="downloadForm" className="downloadForm">
        <button onClick={close}>x</button>
        <label htmlFor="filename">Filename:</label>
        <input type="text" name="filename" className="downloadFilename" placeholder="Filename" value={filename} onChange={(e) => setFilename(e.target.value)} />
        <label htmlFor="format">Format:</label>
        <select name="format" className="downloadFormat" value={format} onChange={(e) => setFormat(e.target.value)}>
          <option value="png">PNG</option>
          <option value="json">JSON (Layout)</option>
        </select>
        {format === "png" && (
          <>
             <label htmlFor="background">Background:</label>
             <select name="background" className="downloadBackground" value={background} onChange={(e) => setBackground(e.target.value)}>
                 <option value="transparent">Transparent</option>
                 <option value="white">White</option>
                 <option value="black">Black</option>
             </select>
          </>
        )}

      <button onClick={handleDownload} className="downloadGraph">Download</button>
    </div>
  );
}
