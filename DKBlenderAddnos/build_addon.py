"""Build a Blender-installable ZIP without Blender or external dependencies."""
from pathlib import Path
import zipfile


def build():
    root = Path(__file__).resolve().parent
    destination = root / "dist" / "dk_exporter.zip"
    destination.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED) as archive:
        for source in sorted((root / "dk_exporter").glob("*.py")):
            archive.write(source, source.relative_to(root).as_posix())
    print(destination)
    return destination


if __name__ == "__main__":
    build()
