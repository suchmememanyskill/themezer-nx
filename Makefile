
.PHONY: all clean

all:
	@$(MAKE) -C JAGL/
	@$(MAKE) -C themezer-nx/

clean:
	@$(MAKE) -C JAGL/ clean
	@$(MAKE) -C themezer-nx/ clean