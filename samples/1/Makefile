MAPCONV=../../mapconv
TEXCOMP=../../texcompress
TEXTURE=texture.png
HEIGHT=height.png
METAL=metal.png
FEATURE=feature.png
FEATURE_LIST=features.txt

main : clean sample1.smf

clean :
	rm -f sample1.sdz
	rm -f sample1.sd7
	rm -Rf maps
	mkdir -p maps
	

sample1.smf: $(MAPCONV) $(TEXCOMP) $(TEXTURE) $(HEIGHT) $(METAL) $(FEATURE) $(FEATURE_LIST)
	$(MAPCONV) -t $(TEXTURE) -a $(HEIGHT) -m $(METAL) -f $(FEATURE) -o maps/$@ -z $(TEXCOMP) -n 10 -x 500 -j $(FEATURE_LIST)
	cp sample1.smd maps/sample1.smd
	7z a sample1.sd7 maps/
	zip -r9 sample1.sdz maps/
	echo "copy either sample.sd7 or sample.sdz to ~/.spring/maps"
	echo "in spring the map is called sample1 then"
	#cp sample1.sd7 ~/.spring/maps

