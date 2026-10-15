#pragma once
#include <complex>
#include <vector>

typedef std::complex<float> Complex;

class FFT {
public:
    void setup(int size);
    void forward(std::vector<Complex>& data) const { transform(data, false); }
    void inverse(std::vector<Complex>& data) const { transform(data, true); }
private:
    void transform(std::vector<Complex>& data, bool inverse) const;
    int n = 0;
    std::vector<int> bitReverse;
    std::vector<Complex> twiddles;
};
