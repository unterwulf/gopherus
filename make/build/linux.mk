mkdir = mkdir $(1)
cp = cp $(1)
rm = rm -f $(1)

# Attention! This conversion presumes that source code is
# checked out using Linux native EOLs (i.e. LF).
ifeq ($(filter $(HOST),win32 dos),)
fixeol = cp $(1) $(2)
else
fixeol = sed 's@$$@\r@' $(1) > $(2)
endif
