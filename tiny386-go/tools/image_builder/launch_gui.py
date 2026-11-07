"""Source double-click and PyInstaller entry point."""
import sys
from pathlib import Path
if not getattr(sys,'frozen',False):
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.image_builder.gui import main

if __name__=='__main__':
    if len(sys.argv)==3 and sys.argv[1]=='--check-runtime':
        from tools.image_builder.diagnostics import check_runtime
        check_runtime(sys.argv[2])
    else: main()
