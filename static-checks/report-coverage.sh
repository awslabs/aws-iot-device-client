# Compile and run the tests before running this script.

lcov --capture --directory ../build/test/CMakeFiles/test-aws-iot-device-client.dir/__/source --output-file coverage.info
genhtml coverage.info --output-directory coverage-out

# To view the results, open the generated index.html file in a web browser.