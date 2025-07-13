import React from 'react';
import { SDCardTree, FileNode } from './SDCardTree';
import { ChunkEnvelope, ChunkReassembler } from './ChunkReassembler';

// Example chunked BLE data matching serial output
const chunkedData: ChunkEnvelope[] = [
  {
    t: "FILE_LIST", s: 1, n: 4, p: "{\"name\":\"/\",\"type\":\"directory\",\"children\":[{\"name\":\".Spotlight-V100\",\"type\":\"directory\"},{\"name\":\".fseventsd\",\"type\":\"di", e: false
  },
  {
    t: "FILE_LIST", s: 2, n: 4, p: "rectory\"},{\"name\":\"._.Spotlight-V100\",\"type\":\"file\",\"size\":4096},{\"name\":\"data.txt\",\"type\":\"file\",\"size\":391},{\"name\":\".", e: false
  },
  {
    t: "FILE_LIST", s: 3, n: 4, p: "_data.txt\",\"type\":\"file\",\"size\":4096},{\"name\":\"sample.txt\",\"type\":\"file\",\"size\":3},{\"name\":\"logs\",\"type\":\"directory\"},{\"", e: false
  },
  {
    t: "FILE_LIST", s: 4, n: 4, p: "name\":\"data2.txt\",\"type\":\"file\",\"size\":391}]}\", e: true
  }
];

export const ChunkedBLEReassembly: React.FC = () => {
  const [fileTree, setFileTree] = React.useState<FileNode | null>(null);

  React.useEffect(() => {
    // Simulate receiving all chunks
    const reassembler = new ChunkReassembler();
    let fullJson: string | null = null;
    for (const chunk of chunkedData) {
      fullJson = reassembler.addChunk(chunk);
      if (fullJson) break;
    }
    if (fullJson) {
      setFileTree(JSON.parse(fullJson));
    }
  }, []);

  return (
    <div style={{ padding: 32, background: '#222', minHeight: '100vh' }}>
      <h3 style={{ color: '#fff' }}>Chunked BLE Reassembly Demo</h3>
      <SDCardTree fileTree={fileTree} onFileSelect={() => {}} />
    </div>
  );
};

export default {
  title: 'SD Card/Chunked BLE Reassembly',
  component: ChunkedBLEReassembly,
}; 