function decodeU16(bytes, offset) {
  return (bytes[offset] << 8) | bytes[offset + 1];
}

function decodeI16(bytes, offset) {
  const value = decodeU16(bytes, offset);
  return value > 0x7fff ? value - 0x10000 : value;
}

function decodeUplink(input) {
  const bytes = input.bytes;

  if (!bytes || bytes.length !== 10) {
    return {
      errors: ["Expected exactly 10 bytes for the aggregate payload."]
    };
  }

  return {
    data: {
      window_id: decodeU16(bytes, 0),
      sample_count: decodeU16(bytes, 2),
      sampling_frequency_hz: decodeU16(bytes, 4) / 10.0,
      dominant_frequency_hz: decodeU16(bytes, 6) / 100.0,
      average_value: decodeI16(bytes, 8) / 1000.0
    }
  };
}
