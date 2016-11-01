<h1> Introduction </h1>
<p> ASON is a lightweight c JSON lib, Only support Linux/OS X. </p>

<h1> Compile </h1>
<p> mkdir build </p>
<p> cd build </p>
<p> cmake -DCMAKE_BUILD_TYPE=Debug .. </p>
<p> make </p>

<h1> Memory Leak Test </h1>
<p> valgrind --leak-check=full ./ason_test </p>

<h1> Acknowledgement </h1>
<span> special thanks to miloYip's </span>
<span>
<a href=" https://zhuanlan.zhihu.com/json-tutorial"> json-tutorial. </a>
</span>
