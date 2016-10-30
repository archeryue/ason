<h1> compile </h1>
<p> mkdir build </p>
<p> cd build </p>
<p> cmake -DCMAKE_BUILD_TYPE=Debug .. </p>
<p> make </p>

<h1> memory leak test </h1>
<p> valgrind --leak-check=full ./ason_test </p>
