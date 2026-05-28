Import("env")

bin_target = env.File("$BUILD_DIR/${PROGNAME}.bin")
env.AlwaysBuild(bin_target)