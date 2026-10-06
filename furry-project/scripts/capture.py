import socket,json,argparse,time,shutil
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('--port',type=int,default=4444)
parser.add_argument('--output',default='qemu-4k.png')
parser.add_argument('--key')
parser.add_argument('--keys',help='Simultaneous qcodes, e.g. shift,f10')
parser.add_argument('--text',help='ASCII text using a US guest keyboard layout')
parser.add_argument('--monitor',help='Read a QEMU human monitor diagnostic')
parser.add_argument('--quit',action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parent.parent
s=socket.create_connection(('127.0.0.1',args.port),10);s.settimeout(20);f=s.makefile('rwb');f.readline()
def command(name,arguments=None):
    message={'execute':name}
    if arguments is not None:message['arguments']=arguments
    f.write((json.dumps(message)+'\n').encode());f.flush()
    while True:
        line=f.readline()
        if not line and name=='quit':return {}
        result=json.loads(line)
        if 'error' in result:raise RuntimeError(result)
        if 'return' in result:return result['return']
command('qmp_capabilities')
name=command('query-name').get('name','')
if name.startswith('refind-furry-lab-'):
    mode=name.removeprefix('refind-furry-lab-')
    for log in Path('/tmp').glob(f'refind-furry-lab-*-{mode}/serial.log'):
        shutil.copyfile(log,root/'evidence'/'phase2'/f'serial-{mode}.log')
if args.monitor:
    print(command('human-monitor-command',{'command-line':args.monitor}))
elif args.key or args.keys:
    command('send-key',{'keys':[{'type':'qcode','data':k} for k in (args.keys or args.key).split(',')]})
elif args.text:
    plain={' ':'spc','\n':'ret','\t':'tab','.':'dot',',':'comma','/':'slash','\\':'backslash','-':'minus','=':'equal',';':'semicolon',"'":'apostrophe','[':'bracket_left',']':'bracket_right','`':'grave'}
    shifted={':':'semicolon','_':'minus','+':'equal','"':'apostrophe','{':'bracket_left','}':'bracket_right','?':'slash','!':'1','>':'dot','<':'comma','|':'backslash','(':'9',')':'0'}
    for c in args.text:
        shift=c.isupper() or c in shifted
        code=shifted.get(c,plain.get(c,c.lower()))
        if not c.isascii() or not (c.isalnum() or c in plain or c in shifted):raise ValueError(f'Unsupported character: {c!r}')
        keys=([{'type':'qcode','data':'shift'}] if shift else [])+[{'type':'qcode','data':code}]
        command('send-key',{'keys':keys,'hold-time':20});time.sleep(.04)
elif args.quit:
    try:command('quit')
    except ConnectionResetError:pass
else:
    output=root/'evidence'/args.output
    output.parent.mkdir(parents=True,exist_ok=True)
    command('screendump',{'filename':str(output.with_suffix('.ppm'))})
    from PIL import Image
    im=Image.open(output.with_suffix('.ppm'));im.save(output)
    im.close();output.with_suffix('.ppm').unlink()
    print(output,im.size)
