cmake_minimum_required (VERSION 2.6)
project (NMEAlib)

include_directories("include")

add_library(nmea SHARED src/context.c src/generate.c src/generator.c src/gmath.c src/info.c src/parse.c src/parser.c src/sentence.c src/time.c src/tok.c)

install (TARGETS nmea DESTINATION lib)
