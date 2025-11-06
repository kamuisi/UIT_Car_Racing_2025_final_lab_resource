import torch 
import torch_tensorrt
from fast_scnn import Fast_SCNN


model = Fast_SCNN(num_classes=5)
model.load_state_dict(torch.load("../model/fast_scnn_model.pth", map_location=torch.device('cuda')))
model.eval().cuda()

inputs = [torch.randn((1, 3, 192, 320)).cuda()] # batch, channels, height, width
traced_model = torch.jit.trace(model, inputs)
trt_gm = torch_tensorrt.compile(traced_model, ir="torchscript", inputs=inputs)
torch.jit.save(trt_gm, "../model/fast_scnn_model.ts")
