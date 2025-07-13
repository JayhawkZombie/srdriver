// ChunkReassembler.ts

export type ChunkEnvelope = {
  t: string; // type
  s: number; // sequence number (1-based)
  n: number; // total number of chunks
  p: string; // payload
  e: boolean; // end of stream
};

export class ChunkReassembler {
  private buffer: Map<number, string> = new Map();
  private expectedTotal: number | null = null;

  addChunk(chunk: ChunkEnvelope): string | null {
    this.buffer.set(chunk.s, chunk.p);
    if (chunk.n) this.expectedTotal = chunk.n;

    // If we have all chunks, reassemble
    if (this.expectedTotal && this.buffer.size === this.expectedTotal) {
      const full = Array.from({ length: this.expectedTotal }, (_, i) => this.buffer.get(i + 1) || '').join('');
      this.buffer.clear();
      this.expectedTotal = null;
      return full;
    }
    return null; // Not done yet
  }

  reset() {
    this.buffer.clear();
    this.expectedTotal = null;
  }
} 