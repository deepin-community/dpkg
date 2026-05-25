/*
 * libdpkg - Debian packaging suite library routines
 * path-rename.c - path rename function
 *
 * Copyright © 2026 UnionTech Software Technology Co., Ltd.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <compat.h>

#include <errno.h>
#include <unistd.h>

#include <dpkg/i18n.h>
#include <dpkg/dpkg.h>
#include <dpkg/path.h>
#include <dpkg/debug.h>
#include <dpkg/subproc.h>

/**
 * Rename a pathname, falling back to move on EXDEV.
 *
 * When rename(2) fails with EXDEV (cross-device link), which commonly
 * happens on OverlayFS where source and destination reside on different
 * layers, this function falls back to using mv(1) which handles
 * cross-filesystem moves automatically.
 *
 * @param oldpath  Source pathname.
 * @param newpath  Destination pathname.
 *
 * @retval 0 On success.
 * @retval -1 On failure, errno is set.
 */
int
path_rename(const char *oldpath, const char *newpath)
{
	if (rename(oldpath, newpath) == 0)
		return 0;

	if (errno != EXDEV)
		return -1;

	/* EXDEV: fall back to mv which handles cross-filesystem moves. */
	debug(_("unable to rename '%.255s' to '%.255s' across filesystems, "
	          "falling back to mv"), oldpath, newpath);

	pid_t pid = subproc_fork();
	if (pid == 0) {
		execlp(MV, "mv", "-f", "--", oldpath, newpath, NULL);
		ohshite(_("unable to execute %s (%s)"),
		        _("mv command for cross-filesystem rename"), MV);
	}
	debug(dbg_eachfile, "%s running mv '%s' '%s' (EXDEV fallback)",
	      __func__, oldpath, newpath);
	if (subproc_reap(pid, _("mv command for cross-filesystem rename"),
	                 SUBPROC_RETERROR)) {
		errno = EIO;
		return -1;
	}

	return 0;
}
