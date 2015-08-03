# Native DOS commands understand paths in DOS notation only, so the
# paths need to be converted before being passed to these commands.
dospath = $(subst /,\,$(1))

mkdir = mkdir $(call dospath,$(1))
cp = copy $(call dospath,$(1))

# The del command doesn't understand multiple arguments. What a pity.
rm = $(foreach file,$(1),del $(call dospath,$(file));) rem

# It's hardly possible that someone do crosscompilation under DOS
# or Windows, so there is no need to fix EOLs as long as source code
# is checked out using DOS/Windows native EOLs (i.e. CRLF).
fixeol = copy $(call dospath,$(1)) $(call dospath,$(2))
