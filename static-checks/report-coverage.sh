# Compile and run the tests before running this script.

cd ../build/test/CMakeFiles/test-aws-iot-device-client.dir/__/source
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory out

# To view the results, open the generated index.html file in a web browser.