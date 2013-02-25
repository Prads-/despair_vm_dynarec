/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "sha256.h"

inline uint32 rotateRight(uint32 x, uint32 y) {
	return (x >> y) | (x << (32 - y));
}

inline uint32 ch(uint32 x, uint32 y, uint32 z) {
	return (x & y) ^ ((~x) & z);
}

inline uint32 maj(uint32 x, uint32 y, uint32 z) {
	return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32 upperSigma0(uint32 x) {
	return rotateRight(x, 2) ^ rotateRight(x, 13) ^ rotateRight(x, 22);
}

inline uint32 upperSigma1(uint32 x) {
	return rotateRight(x, 6) ^ rotateRight(x, 11) ^ rotateRight(x, 25);
}

inline uint32 lowerSigma0(uint32 x) {
	return rotateRight(x, 7) ^ rotateRight(x, 18) ^ (x >> 3);
}

inline uint32 lowerSigma1(uint32 x) {
	return rotateRight(x, 17) ^ rotateRight(x, 19) ^ (x >> 10);
}

SHA256::SHA_256_MessageDigest SHA256::sha256(const uint8 *message, uint32 mSize) {
	//Preprocessing
	int paddingOffset = (mSize % 64), paddingSize;
	uint8 *padding;
	int numOfMs;

	if (paddingOffset >= 56) {
		paddingSize = 64 + 64;
	} else { 
		paddingSize = 64;
	}

	numOfMs = (mSize / 64) + (paddingSize / 64);
	uint32 **m = new uint32 *[numOfMs];

	padding = new uint8[paddingSize];
	memset(padding, 0, paddingSize);
	memcpy(padding, &message[mSize - paddingOffset], paddingOffset);
	padding[paddingOffset] = 0x80;
	uint64 l = mSize * 8;

	//I think they made number of bits big endian for a reason...
	uint8 *lBytes = (uint8*)&l;
	for (int i = 0; i < 4; ++i) {
		lBytes[i] ^= lBytes[~i & 7];
		lBytes[~i & 7] ^= lBytes[i];
		lBytes[i] ^= lBytes[~i & 7];
	}

	*(uint64*)&padding[paddingSize - 8] = l;

	const uint8 *msgPtr = message;
	for (int i = 0, counter = 0; i < numOfMs; ++i, counter += 64) {
		if ((mSize - counter) >= 64) {
			m[i] = (uint32*)&msgPtr[counter];
		} else {
			msgPtr = padding;
			mSize += paddingSize;
			counter = 0;
			m[i] = (uint32*)&msgPtr[counter];
		}
	}

	const uint32 k[] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
						0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
						0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
						0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
						0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
						0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
						0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
						0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

	SHA_256_MessageDigest digest;
	
	digest.h[0] = 0x6a09e667;
	digest.h[1] = 0xbb67ae85;
	digest.h[2] = 0x3c6ef372;
	digest.h[3] = 0xa54ff53a;
	digest.h[4] = 0x510e527f;
	digest.h[5] = 0x9b05688c;
	digest.h[6] = 0x1f83d9ab;
	digest.h[7] = 0x5be0cd19;

	uint32 w[64];
	uint32 a, b, c, d, e, f, g, h;
	uint32 t1, t2;

	//SHA-256 computation
	for (int i = 0; i < numOfMs; ++i) {
		for (int t = 0 ; t <= 15; ++t) {
			w[t] = m[i][t];
		}
		for (int t = 16; t <= 63; ++t) {
			w[t] = lowerSigma1(w[t - 2]) + w[t-7] + lowerSigma0(w[t - 15]) + w[t - 16];
		}

		a = digest.h[0];
		b = digest.h[1];
		c = digest.h[2];
		d = digest.h[3];
		e = digest.h[4];
		f = digest.h[5];
		g = digest.h[6];
		h = digest.h[7];

		for (int t = 0; t <= 63; ++t) {
			t1 = h + upperSigma1(e) + ch(e, f, g) + k[t] + w[t];
			t2 = upperSigma0(a) + maj(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + t1;
			d = c;
			c = b;
			b = a;
			a = t1 + t2;
		}

		digest.h[0] = a + digest.h[0];
		digest.h[1] = b + digest.h[1];
		digest.h[2] = c + digest.h[2];
		digest.h[3] = d + digest.h[3];
		digest.h[4] = e + digest.h[4];
		digest.h[5] = f + digest.h[5];
		digest.h[6] = g + digest.h[6];
		digest.h[7] = h + digest.h[7];
	}

	delete [] padding;
	delete [] m;
	return digest;
}