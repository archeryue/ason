<h> compile </h>
<p> mkdir build </p>
<p> cd build </p>
<p> cmake -DCMAKE_BUILD_TYPE=Debug .. </p>
<p> make </p>

<h> memory leak test </h>
<p> valgrind --leak-check=full ./ason_test </p>
