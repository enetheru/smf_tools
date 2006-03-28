MAPCONV=../../mapconv
TEXCOMP=../../texcompress
TEXTURE=ground.png
HEIGHT=height.png
METAL=metal.png
FEATURE=features.png
FEATURE_LIST=fs.txt

tiny.smf: $(MAPCONV) $(TEXCOMP) $(TEXTURE) $(HEIGHT) $(METAL) $(FEATURE) $(FEATURE_LIST)
	$(MAPCONV) -t $(TEXTURE) -a $(HEIGHT) -m $(METAL) -f $(FEATURE) -o $@ -z $(TEXCOMP) -n 50 -x 165