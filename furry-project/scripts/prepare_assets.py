"""Prepare runtime resources without ever drawing hardware text into the banner."""
from pathlib import Path
from PIL import Image
import shutil
root=Path(__file__).resolve().parent.parent
reference=root/'theme-reference/refind-finn-term-main'
theme=root/'theme'
shutil.copytree(reference,theme,dirs_exist_ok=True)
Image.open(root/'assets/furry-source.png').convert('RGB').resize((3840,2160),Image.Resampling.LANCZOS).save(theme/'background.png')
for name,size in [('selection_big.png',384),('selection_small.png',144)]:
    Image.open(reference/name).resize((size,size),Image.Resampling.LANCZOS).save(theme/name)
im=Image.open(reference/'fonts/jetbrains-mono-30.png').convert('RGBA')
im=im.resize((im.width*3//2,im.height*3//2),Image.Resampling.LANCZOS)
alpha=im.getchannel('A');atlas=Image.new('RGBA',im.size,(59,74,2,0));atlas.putalpha(alpha)
atlas.save(theme/'fonts/jetbrains-mono-45.png')
for f in (theme/'icons').glob('*.png'):
    Image.open(f).convert('RGBA').save(f)
s=(reference/'theme.conf').read_text().replace('30.png','45.png').replace('2560 1440','3840 2160').replace('big_icon_size 256','big_icon_size 384').replace('small_icon_size 96','small_icon_size 144')
(theme/'theme.conf').write_text(s+'\ndynamic_info true\n',encoding='utf-8',newline='\n')
print('Prepared 3840x2160 banner and theme assets; no hardware strings baked in.')
