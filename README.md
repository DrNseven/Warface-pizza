# Warface-pizza
simple d3d multihack using crc and pic esp (target.png in Release directory)

pizza ingredients:
- grab models using GetDeclaration (compatibility)
- get coordinates from shader
- get crc of warface/blackwood signs 
- pic esp without fps loss

how to use:
- compile dll (in x86 mode)
- launch warface, inject wfbot.dll into game.exe
- press INSERT to toggle menu
- use arrows to navigate
- set scope to aimkey for best accuracy
 
note to self, full model chams:
texCRC == 0x4cb78f85
Device->SetTexture(0, texBlackwood);

texCRC == 0x634b8fce
Device->SetTexture(0, texWarface);
