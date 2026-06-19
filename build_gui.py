#!/usr/bin/env python3
"""OrangeX Kernel Build GUI - CLI Edition"""
import os, sys, shutil, subprocess, threading, tkinter as tk
from tkinter import scrolledtext, messagebox

SCRIPT = os.path.dirname(os.path.abspath(__file__))
KERN_BIN = os.path.join(SCRIPT, "orangex_kernel.bin")
KERN_ELF = os.path.join(SCRIPT, "orangex_kernel.elf")
W64 = r"C:\w64devkit\bin"
QEMU_DEFAULT = r"C:\Program Files\qemu\qemu-system-i386.exe"
PYTHON_DIR = r"C:\Users\samrat\AppData\Local\Programs\Python\Python312"

for d in [W64, PYTHON_DIR]:
    if os.path.isdir(d) and d not in os.environ.get("PATH", ""):
        os.environ["PATH"] = d + ";" + os.environ.get("PATH", "")

def find_make():
    p = shutil.which("make")
    if p: return p
    for c in [W64 + r"\make.exe"]:
        if os.path.isfile(c): return c
    return "make"

def find_qemu():
    p = shutil.which("qemu-system-i386")
    if p: return p
    for c in [QEMU_DEFAULT]:
        if os.path.isfile(c): return c
    return None

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("OrangeX Kernel v1.0")
        self.root.geometry("780x560")
        self.root.resizable(False, False)
        self.root.configure(bg="#1a1b26")
        self.building = False
        self.running = False
        self._ui()

    def _ui(self):
        f = tk.Frame(self.root, bg="#1a1b26")
        f.pack(fill=tk.X, padx=12, pady=(10,0))
        tk.Label(f, text="OrangeX Kernel v1.0",
                 font=("Consolas",16,"bold"), fg="#f7768e", bg="#1a1b26").pack(side=tk.LEFT)
        self.status = tk.Label(f, text="Ready", font=("Consolas",10), fg="#9ece6a", bg="#1a1b26")
        self.status.pack(side=tk.RIGHT)

        bf = tk.Frame(self.root, bg="#1a1b26")
        bf.pack(fill=tk.X, padx=12, pady=8)

        bf2 = tk.Frame(bf, bg="#1a1b26")
        bf2.pack(fill=tk.X)

        self.btn_build = tk.Button(bf2, text="Build Kernel",
            font=("Consolas",11,"bold"), fg="#1a1b26", bg="#9ece6a",
            activebackground="#73daca", relief=tk.FLAT, padx=14, pady=5,
            command=self._build)
        self.btn_build.pack(side=tk.LEFT, padx=(0,6))

        self.btn_run = tk.Button(bf2, text="Launch in QEMU",
            font=("Consolas",11,"bold"), fg="#1a1b26", bg="#7aa2f7",
            activebackground="#7dcfff", relief=tk.FLAT, padx=14, pady=5,
            command=self._run)
        self.btn_run.pack(side=tk.LEFT, padx=(0,6))

        self.btn_clean = tk.Button(bf2, text="Clean",
            font=("Consolas",11), fg="#c0caf5", bg="#414868",
            activebackground="#565f89", relief=tk.FLAT, padx=14, pady=5,
            command=self._clean)
        self.btn_clean.pack(side=tk.LEFT)

        tk.Frame(self.root, bg="#292e42", height=1).pack(fill=tk.X, padx=12, pady=6)

        tk.Label(self.root, text="Log:", font=("Consolas",9), fg="#565f89", bg="#1a1b26", anchor=tk.W).pack(fill=tk.X, padx=12)
        self.log = scrolledtext.ScrolledText(self.root, font=("Consolas",9),
            bg="#1a1b26", fg="#c0caf5", insertbackground="#c0caf5",
            relief=tk.FLAT, wrap=tk.WORD, height=18)
        self.log.pack(fill=tk.BOTH, expand=True, padx=12, pady=(2,10))
        self.log.configure(state=tk.DISABLED)
        self._log("OrangeX Kernel Build System v1.0")
        self._log("Click 'Build Kernel' then 'Launch in QEMU'")
        self._log("")

    def _log(self, m):
        self.log.configure(state=tk.NORMAL)
        self.log.insert(tk.END, m + "\n")
        self.log.see(tk.END)
        self.log.configure(state=tk.DISABLED)

    def _log_clear(self):
        self.log.configure(state=tk.NORMAL)
        self.log.delete("1.0", tk.END)
        self.log.configure(state=tk.DISABLED)

    def _status(self, t, c):
        self.status.config(text=t, fg=c)

    def _build(self):
        if self.building: return
        self.building = True
        self.btn_build.config(state=tk.DISABLED)
        self.btn_run.config(state=tk.DISABLED)
        self._status("Building...", "#e0af68")
        self._log_clear()
        self._log("=== Building Kernel ===")
        threading.Thread(target=self._do_build, daemon=True).start()

    def _do_build(self):
        mk = find_make()
        self.root.after(0, self._log, f"Using: {mk}")
        try:
            r = subprocess.run([mk, "clean"], cwd=SCRIPT, capture_output=True, text=True, timeout=30)
            if r.stdout.strip(): self.root.after(0, self._log, r.stdout.strip())
        except Exception as e:
            self.root.after(0, self._log, f"clean: {e}")

        self.root.after(0, self._log, "\n--- Compiling ---")
        try:
            r = subprocess.run([mk, "all"], cwd=SCRIPT, capture_output=True, text=True, timeout=120)
            if r.stdout.strip(): self.root.after(0, self._log, r.stdout.strip())
            if r.returncode != 0:
                err = r.stderr.strip() if r.stderr else "Unknown"
                self.root.after(0, self._log, f"\nBUILD FAILED (exit {r.returncode})")
                self.root.after(0, self._log, err)
                self.root.after(0, self._done, False)
                return
        except Exception as e:
            self.root.after(0, self._log, f"compile: {e}")
            self.root.after(0, self._done, False)
            return

        if os.path.isfile(KERN_BIN):
            sz = os.path.getsize(KERN_BIN)
            self.root.after(0, self._log, f"\nBUILD SUCCESS: orangex_kernel.bin ({sz} bytes)")
            self.root.after(0, self._done, True)
        else:
            self.root.after(0, self._log, "\nKernel binary not found.")
            self.root.after(0, self._done, False)

    def _done(self, ok):
        self.building = False
        self.btn_build.config(state=tk.NORMAL)
        self.btn_run.config(state=tk.NORMAL)
        self._status("Build Succeeded" if ok else "Build Failed", "#9ece6a" if ok else "#f7768e")

    def _run(self):
        if self.running: return
        qemu = find_qemu()
        if not qemu:
            messagebox.showerror("QEMU Not Found", "Install QEMU from:\nhttps://www.qemu.org/download/#windows")
            return
        if not os.path.isfile(KERN_ELF):
            messagebox.showwarning("No Kernel", "Build the kernel first.")
            return
        self.running = True
        self._status("Running...", "#7aa2f7")
        self._log(f"\nLaunching: {qemu} -kernel orangex_kernel.elf -m 128M")
        threading.Thread(target=self._do_run, args=([qemu, "-kernel", KERN_ELF, "-m", "128M"],), daemon=True).start()

    def _do_run(self, cmd):
        try:
            proc = subprocess.Popen(cmd, cwd=SCRIPT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.root.after(0, self._log, f"QEMU started (PID {proc.pid})")
            proc.wait()
            self.root.after(0, self._log, "QEMU exited.")
        except FileNotFoundError:
            self.root.after(0, self._log, "ERROR: QEMU not found.")
        except Exception as e:
            self.root.after(0, self._log, f"Error: {e}")
        finally:
            self.running = False
            self.root.after(0, self._status, "Ready", "#9ece6a")

    def _clean(self):
        if self.building: return
        mk = find_make()
        try:
            subprocess.run([mk, "clean"], cwd=SCRIPT, capture_output=True, text=True, timeout=30)
            self._log("Clean complete.")
        except Exception as e:
            self._log(f"clean: {e}")

def main():
    root = tk.Tk()
    App(root)
    root.mainloop()

if __name__ == "__main__":
    main()
