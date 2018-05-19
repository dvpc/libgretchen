import matplotlib.pyplot as plt
import numpy as N
import wave

def extract_float32_from_binary_file(filename):
    f = open(filename, 'rb')
    d = f.read()
    d = N.fromstring(d, 'float32')
    f.close()
    return d

n1 = extract_float32_from_binary_file('tmp.output.raw')
n2 = extract_float32_from_binary_file('tmp.output2.raw')
n3 = extract_float32_from_binary_file('tmp.output3.raw')

# import pdb; pdb.set_trace()

plt.figure(1)

plt.subplot(311)
plt.title('org')
plt.plot(n1, linewidth=.1)

plt.subplot(312)
plt.title('noise')
plt.plot(n2, linewidth=.1)

plt.subplot(313)
plt.title('denoise')
plt.plot(n3, linewidth=.1)

plt.show()
