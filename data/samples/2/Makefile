MAPCONV=../../mapconv
TEXCOMP=../../texcompress
TEXTURE=ground.png
HEIGHT=height.png
METAL=metal.png
FEATURE=features.png
FEATURE_LIST=fs.txt
GEOVENT=../../geovent.bmp

tiny.smf: $(MAPCONV) $(TEXCOMP) $(TEXTURE) $(HEIGHT) $(METAL) $(FEATURE) $(FEATURE_LIST) $(GEOVENT)
	$(MAPCONV) -t $(TEXTURE) -a $(HEIGHT) -m $(METAL) -f $(FEATURE) -o $@ -z $(TEXCOMP) -n 50 -x 165 -g $(GEOVENT) 