CJK_FONT=Noto Sans Mono CJK SC
VAR_FONT=$(shell if fc-list :lang=zh | grep '$(CJK_FONT)' >/dev/null; then echo "-V CJKmainfont='$(CJK_FONT)'"; fi)
PD_FLAGS=--pdf-engine=xelatex -N ${VAR_FONT}
SOURCES=$(wildcard *.md)
TARGETS=$(patsubst %.md, %.pdf, $(SOURCES))

all: $(TARGETS)

%.pdf: %.md
	pandoc $(PD_FLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.pdf
