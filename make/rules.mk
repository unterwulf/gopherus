all: $(prog)

pkg: $(pkgfiles)

$(objdir) $(pkgdir):
	$(call mkdir,$@)


$(objs): | $(objdir)

$(objdir)/%.o: $(srcdir)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -c -o $@ $<

$(objdir)/%.res: $(srcdir)/%.rc
	$(WINDRES) $< -O coff -o $@

$(prog): $(objs)
	$(CC) $(CFLAGS) $^ $(libs) -o $@


$(pkgfiles): | $(pkgdir)

$(pkgdir)/$(progname): $(objdir)/$(progname)
	$(call rm,$@)
	$(UPX) --best --lzma --all-filters -o $@ $<

$(pkgdir)/gopherus.svg: $(srcdir)/icon.svg
	$(call cp,$< $@)

$(pkgdir)/SDL.dll: $(srcdir)/SDL.dll
	$(call cp,$< $@)

$(pkgdir)/%.ico: $(srcdir)/%.ico
	$(call cp,$< $@)

$(pkgdir)/%.txt: $(srcdir)/%.txt
	$(call fixeol,$<,$@)

clean:
	$(call rm,$(objs))
	$(call rm,$(prog))
	$(call rm,$(pkgfiles))
