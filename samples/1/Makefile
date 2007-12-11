MAPCONV=../../mapconv
TEXCOMP=../../texcompress
TEXTURE=texture.png
HEIGHT=height.png
METAL=metal.png
FEATURE=feature.png
FEATURE_LIST=features.txt

test.smf: $(MAPCONV) $(TEXCOMP) $(TEXTURE) $(HEIGHT) $(METAL) $(FEATURE) $(FEATURE_LIST)
	$(MAPCONV) -t $(TEXTURE) -a $(HEIGHT) -m $(METAL) -f $(FEATURE) -o $@ -z $(TEXCOMP) -n 10 -x 500 -j $(FEATURE_LIST)
