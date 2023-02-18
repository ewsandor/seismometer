clear;
clc;
pkg load signal;

function window = hamming_window(N,a_0=25/46)
  a_1=1-a_0;
  n=0:N-1;
  window = (a_0-a_1*cos((2*pi*n)/N));
end

function window = blackman_window(N, alpha=0)
  if(alpha > 0)
    a_0=(1-alpha)/2;
    a_1=1/2;
    a_2=alpha/2;
  else
    a_0=7938/18608;
    a_1=9240/18608;
    a_2=1430/18608;
  end


  window = (a_0-a_1*cos((2*pi*n)/N)+a_2*cos((4*pi*n)/N));
end

Fs=100;
Ts=1/Fs;
cutoff_f=10;

N=64;
n=0:N-1;
t=n*Ts;
f=n*Fs/N;

output_filter=sinc((cutoff_f*2/Fs)*(n-N/2))*(cutoff_f*2);
#plot(abs(fft(output_filter)))

window=ones(1,N);
window=hamming(N)';
#window=blackman(N)';
output_filter=output_filter.*window;
freqz(output_filter, fs=Fs);
