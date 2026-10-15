#include "core/FFT.h"
#include "core/Utils.h"

void FFT::setup(int size) {
    n = size;
    int bits = 0;
    while ((1 << bits) < n) bits++;
    bitReverse.resize(n);
    for (int i = 0; i < n; i++) {
        int r = 0;
        for (int b = 0; b < bits; b++) {
            if (i & (1 << b)) r |= 1 << (bits - 1 - b);
        }
        bitReverse[i] = r;
    }
    twiddles.resize(n / 2);
    for (int i = 0; i < n / 2; i++) twiddles[i] = std::polar(1.0f, (float)(-2.0 * PI * i / n));
}

void FFT::transform(std::vector<Complex>& data, bool inverse) const {
    for (int i = 0; i < n; i++) {
        if (i < bitReverse[i]) std::swap(data[i], data[bitReverse[i]]);
    }
    for (int len = 2; len <= n; len *= 2) {
        int half = len / 2;
        int stride = n / len;
        for (int start = 0; start < n; start += len) {
            for (int k = 0; k < half; k++) {
                Complex w = twiddles[k * stride];
                if (inverse) w = std::conj(w);
                Complex a = data[start + k];
                Complex b = data[start + k + half] * w;
                data[start + k] = a + b;
                data[start + k + half] = a - b;
            }
        }
    }
    if (inverse) {
        for (int i = 0; i < n; i++) data[i] /= (float)n;
    }
}
