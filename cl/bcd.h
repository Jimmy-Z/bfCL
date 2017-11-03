
#ifdef BCD

inline u64 to_dsi_bcd(u64 i) {
	// 234 -> 0x234
	u64 o = 0;
	unsigned shift = 0;
	while (i) {
		o |= (i % 10) << (shift++ << 2);
		i /= 10;
	}
	// 0x234 -> 0x2134
	return ((o & ~0xff) << 4) | 0x100 | (o & 0xff);
}

#endif // BCD
