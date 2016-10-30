<h1> Introduction </h1>
<p> ASON is a light weight c JSON lib. </p>
<p> I code it under milo's online guidance. </p>
<p> online guidance's website: https://zhuanlan.zhihu.com/json-tutorial </p>

<h1> Compile </h1>
<p> mkdir build </p>
<p> cd build </p>
<p> cmake -DCMAKE_BUILD_TYPE=Debug .. </p>
<p> make </p>

<h1> Memory Leak Test </h1>
<p> valgrind --leak-check=full ./ason_test </p>
