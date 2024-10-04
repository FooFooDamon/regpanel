#
# Copyright (c) 2024 Man Hung-Coeng <udc577@126.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

.PHONY: all prepare dependencies

ifeq ($(shell [ -s qt_print.hpp -a -s __ver__.mk -a -s QtMakefile ] && echo 1 || echo 0),0)

LAZY_CODING_URL ?= https://github.com/FooFooDamon/lazy_coding_skills

all prepare: dependencies
	@[ -s qt_print.hpp ] || wget -c "${LAZY_CODING_URL}/raw/main/c_and_cpp/native/qt_print.hpp"
	@[ -s __ver__.mk ] || wget -c "${LAZY_CODING_URL}/raw/main/makefile/__ver__.mk"
	@[ -s QtMakefile ] || qmake -o QtMakefile
	@echo "~ ~ ~ Minimum preparation finished successfully ~ ~ ~"
	@echo "Re-run your command again to continue your work."

else

all: dependencies

include __ver__.mk
include QtMakefile

DEFINES += -D__VER__=\"${__VER__}\" -DHAS_LOGGER
CFLAGS += -Wno-unused-parameter
CXXFLAGS += -Wno-unused-parameter
INCPATH +=
LFLAGS +=
LIBS +=
NO_CPPCHECK = true
OS_MACRO ?= -D__linux__
PREDEFS_FOR_CPPCHECK ?= $(if $(wildcard moc_predefs.h), --include=moc_predefs.h, \
    ${OS_MACRO} $$(g++ -dM -E - < /dev/null | grep ENDIAN | awk '{ printf("-D%s=%s\n", $$2, $$3) }'))

.PHONY: ext_clean check debug_patch ui_fix

clean: ext_clean

ext_clean:
	rm -f ${TARGET} *.d *.plist

check:
	-${NO_CPPCHECK} && printf "\n[Warning] Cppcheck has been disabled since it consumes too much time!\n%s\n\n" \
		"If you want to enable it, run with NO_CPPCHECK=false" >&2 \
		|| cppcheck --quiet --force --enable=all -j $$(nproc) --language=c++ --std=c++11 \
		--library=qt ${PREDEFS_FOR_CPPCHECK} \
		${DEFINES} ${INCPATH} $(filter-out moc_%.cpp, ${SOURCES})
	clang --analyze $(filter-out moc_${TARGET}.cpp, ${SOURCES}) ${CXXFLAGS} ${INCPATH}

debug_patch:
	@if [ -s ${TARGET}.debug.pri ]; then \
		echo "${TARGET}.debug.pri already exists." >&2; \
	else \
		echo "CONFIG += debug" > ${TARGET}.debug.pri; \
		${MAKE} distclean; \
		printf "\n~ ~ ~ ${TARGET}.debug.pri was generated successfully. ~ ~ ~\n\nYou can re-build the project now.\n\n"; \
	fi

ui_fix: ui_${TARGET}.h
	sed -i 's/\(.*\<QPalette::PlaceholderText\>.*\)/\/\/\1/' $<

__prepare_parent_dirs:
	mkdir -p ${DESTDIR}/usr/local/bin ${DESTDIR}/usr/local/etc

install: __prepare_parent_dirs
	cp ${TARGET} ${DESTDIR}/usr/local/bin/
	@if [ -e ${DESTDIR}/usr/local/etc/${TARGET} ]; then \
		echo "*** Directory or link already exists: ${DESTDIR}/usr/local/etc/${TARGET}" >&2; \
		echo "Check if it's installed by another project, and handle it manually before this installation." >&2; \
		false; \
	else \
		set -x; ln -s $$(realpath $${PWD}/../etc) ${DESTDIR}/usr/local/etc/${TARGET}; set +x; \
		printf "\nConfiguration files in this project are no longer updated,\n"; \
		echo "please do one more step to install another project instead:"; \
		printf "\nhttps://github.com/FooFooDamon/regpanel-conf\n\n"; \
	fi

uninstall: __prepare_parent_dirs
	rm -f ${DESTDIR}/usr/local/bin/${TARGET}
	@[ -e ${DESTDIR}/usr/local/etc/${TARGET} ] || exit 0; \
	actual_dir_path="$$(realpath ${DESTDIR}/usr/local/etc/${TARGET})"; \
	if [ "$${actual_dir_path}" = "$$(realpath $${PWD}/../etc)" ]; then \
		set -x; rm ${DESTDIR}/usr/local/etc/${TARGET}; \
	else \
		echo "Ignored configuration installed by another project: $${actual_dir_path}" >&2; \
	fi

endif

export DEPENDENCY_DIRS ?= $(abspath ../3rdparty)

dependencies:
	@for i in ${DEPENDENCY_DIRS}; \
	do \
		[ -s $${i}/[Mm]akefile ] && ${MAKE} $(filter all prepare, ${MAKECMDGOALS}) -C $${i} || true; \
	done

