echo "running"
# -DNDEBUG due to pugixml assert
clang++ -DNDEBUG -arch arm64 -std=c++17 -I./dependencies/skia/ -L./dependencies/skia/lib   -lskshaper -lskunicode -lskparagraph -lskia -framework CoreServices -framework CoreGraphics -framework CoreText -framework CoreFoundation -framework Metal -framework Foundation -framework QuartzCore $1 src/pugixml.cpp -o $2


