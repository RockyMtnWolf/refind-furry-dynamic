import socket,json,argparse,time
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=4444);parser.add_argument('--output',default='qemu-4k.png');parser.add_argument('--key');parser.add_argument('--quit',action='store_true');args=parser.parse_args()
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
if args.key:command('send-key',{'keys':[{'type':'qcode','data':args.key}]})
elif args.quit:
    try:command('quit')
    except ConnectionResetError:pass
else:
    output=root/'evidence'/args.output
    command('screendump',{'filename':str(output.with_suffix('.ppm'))})
    from PIL import Image
    im=Image.open(output.with_suffix('.ppm'));im.save(output)
    print(output,im.size)
