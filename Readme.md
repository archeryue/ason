<h1> introduction </h1>
<p>ASON is a light weight c JSON lib.</p>
<p>I code it by milo's online guidance.</p>
<p>online guidance's website: https://zhuanlan.zhihu.com/json-tutorial</p>

<h1> compile </h1>
<p> mkdir build </p>
<p> cd build </p>
<p> cmake -DCMAKE_BUILD_TYPE=Debug .. </p>
<p> make </p>

<h1> memory leak test </h1>
<p> valgrind --leak-check=full ./ason_test </p>
