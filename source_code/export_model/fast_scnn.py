import torch
from torch import nn
import torch.nn.functional as F


class DepthwiseSeperableConv(nn.Module):
    def __init__(self, in_channels, out_channels, stride=1):
        super().__init__()
        self.depthwise = nn.Conv2d(in_channels, in_channels, kernel_size=3, 
                                stride=stride, padding=1, groups=in_channels, bias=False)
        self.dw_bn = nn.BatchNorm2d(in_channels)
        self.pointwise = nn.Conv2d(in_channels, out_channels, kernel_size=1, stride=1, bias=False)
        self.pw_bn = nn.BatchNorm2d(out_channels)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
       x = self.relu(self.dw_bn(self.depthwise(x)))
       x = self.relu(self.pw_bn(self.pointwise(x)))
       return x

class Bottleneck(nn.Module):
    def __init__(self, in_channels, out_channels, expansion, stride):
      super().__init__()
      hidden_dim = in_channels * expansion
      self.use_res_connect = stride == 1 and in_channels == out_channels
      self.block = nn.Sequential(
        nn.Conv2d(in_channels, hidden_dim, 1, bias=False),
        nn.BatchNorm2d(hidden_dim),
        nn.ReLU(inplace=True),
        nn.Conv2d(hidden_dim, hidden_dim, 3, stride, 1, groups=hidden_dim,
                   bias=False),
        nn.BatchNorm2d(hidden_dim),
        nn.ReLU(inplace=True),
        nn.Conv2d(hidden_dim, out_channels, 1, bias=False),
        nn.BatchNorm2d(out_channels)
      )
    def forward(self, x):
        if self.use_res_connect:
           return x + self.block(x)
        else:
           return self.block(x) 


class PyramidPoolingModule(nn.Module):
    def __init__(self, in_channels, pool_sizes=(1, 2, 3, 6)):
        super().__init__()
        out_channels = in_channels // len(pool_sizes)
        self.stages = nn.ModuleList([
            nn.Sequential(
                nn.AdaptiveAvgPool2d(ps),
                nn.Conv2d(in_channels, out_channels, kernel_size=1, bias=False),
                nn.BatchNorm2d(out_channels),
                nn.ReLU(inplace=True)
            )
            for ps in pool_sizes
        ])
        self.bottleneck = nn.Sequential(
            nn.Conv2d(256, 128, kernel_size=1, bias=False),
            nn.BatchNorm2d(128),
            nn.ReLU(inplace=True)
        )
    
    def forward(self, x):
        h, w = x.size(2), x.size(3)
        out = [x]
        for stage in self.stages:
            pooled = stage(x)
            upsampled = F.interpolate(pooled, size=(h, w), mode='bilinear', align_corners=False)
            out.append(upsampled)
        out = torch.cat(out, dim=1)
        out = self.bottleneck(out)
        return out

class FeatureFusionModule(nn.Module):
    def __init__(self, high_res_channels, low_res_channels, out_channels, scale_factor):
        super().__init__()
        self.scale_factor = scale_factor

        self.high_res_conv = nn.Sequential(
            nn.Conv2d(high_res_channels, out_channels, kernel_size=1, bias=False),
            nn.BatchNorm2d(out_channels)
        )

        self.low_res_conv = nn.Sequential(
            nn.Conv2d(low_res_channels, out_channels, kernel_size=1, bias=False),
            nn.BatchNorm2d(out_channels)
        )

        self.dwconv = nn.Sequential(
            nn.Conv2d(out_channels, out_channels, kernel_size=3, padding=scale_factor,
                      dilation=scale_factor, groups=out_channels, bias=False),
            nn.BatchNorm2d(out_channels),
            nn.ReLU(inplace=True)
        )

        self.relu = nn.ReLU(inplace=True)

    def forward(self, high_res_input, low_res_input):
        high = self.high_res_conv(high_res_input)
        low = F.interpolate(low_res_input, scale_factor=float(self.scale_factor),
                            mode='bilinear', align_corners=False)
        low = self.dwconv(low)
        low = self.low_res_conv(low)

        out = high + low
        return self.relu(out)

class Fast_SCNN(nn.Module):
    def __init__(self, num_classes):
        super().__init__()
        self.downsample = nn.Sequential(
            nn.Conv2d(3, 32, 3, stride=2, padding=1, bias=False),
            nn.BatchNorm2d(32),
            nn.ReLU(inplace=True),
            DepthwiseSeperableConv(32, 48, stride=2),
            DepthwiseSeperableConv(48, 64, stride=2)
        )

        self.gobal_feature = nn.Sequential(
            Bottleneck(64, 64, expansion=6, stride=2),
            Bottleneck(64, 64, expansion=6, stride=1),
            Bottleneck(64, 64, expansion=6, stride=1),

            Bottleneck(64, 96, expansion=6, stride=2),
            Bottleneck(96, 96, expansion=6, stride=1),
            Bottleneck(96, 96, expansion=6, stride=1),
        
            Bottleneck(96, 128, expansion=6, stride=1),
            Bottleneck(128, 128, expansion=6, stride=1),
            Bottleneck(128, 128, expansion=6, stride=1),

            PyramidPoolingModule(128)
        )

        self.fusion = FeatureFusionModule(64, 128, 128, 4)

        self.classifier = nn.Sequential(
            DepthwiseSeperableConv(128, 128),
            DepthwiseSeperableConv(128, 128),
            nn.Dropout2d(p=0.1),
            nn.Conv2d(128, num_classes, kernel_size=1)
        )

    def forward(self, x):
        size = x.size()[2:]

        down = self.downsample(x)

        global_feat = self.gobal_feature(down)

        fused = self.fusion(down, global_feat)

        out = self.classifier(fused)
        out = F.interpolate(out, size=size, mode='bilinear', align_corners=False)
        return out