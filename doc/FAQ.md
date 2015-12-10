Frequently Asked Questions
==========================

This is a collection of questions that may arise when looking at the code. It's
primarly targeted at developers who may look at a piece of code and wonder:
"hey, this is weird, I'll fix that!", while in fact there is a deeper reason
*why* that code is so weird.


* **Why not start managing windows in `createnotify` instead of `mapnotify`?**

  This is due to some applications not mapping their windows immediately after
  creating them. This would cause awkward holes in the window layout, as we only
  handle mapped windows.

* **Why not stop managing windows in `unmapnotify` instead of `destroynotify`?**

  This is due to desktop switching: we unmap all windows from invisible
  desktops, so if we would stop managing all unmapped windows there, we would
  basically lose track of all desktops we leave. That's not nice.

* **Why do we catch some X errors?**

  In a perfect world, each X error could be avoided by having a perfect program.
  Unfortunately, some X errors cannot be avoided. Here a breakdown of all of
  them, and why we catch them before they wreak havoc:

    * `error_code == BadWindow`: This error is caused by clients, no matter
      what. We don't know why. Anyway, it's beyond the control of karuiwm.

    * `request_code == X_SetInputFocus && error_code == BadMatch`: This error is
      caused if we focus a window after it has been unmapped/destroyed.
      Sometimes, several windows get unmapped at the same time, but we cannot
      know of the second unmapped window until we have handled the first one.
      This is often the case when a dialog box asks for confirmation to close an
      application.

  Of course, catching an X error will hide whenever there *truly* is an issue in
  our code. But we can't change that. If someone finds an elegant way to
  circumvent catching any of the errors above, (s)he is welcome to solve that.

* **Why is there a `seltiled`?**

  The purpose of keeping a pointer to the selected tiled client is to be able to
  put it on the top of the stack when arranging tiled windows. This may seem
  unnecessary, but when applying layouts that overlap clients (such as the
  monocle layout), we would like to keep the focused client on top.
