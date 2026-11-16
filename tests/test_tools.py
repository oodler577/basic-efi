import importlib.util
import os
import pathlib
import tempfile
import unittest
from unittest.mock import patch

ROOT=pathlib.Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('image_builder',ROOT/'tools/create_image.py')
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)

class ImageSafetyTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.folder=pathlib.Path(self.temp.name);self.efi=self.folder/'INPUT.EFI';self.efi.write_bytes(b'INPUT')
        self.target=self.folder/'disk.img'
    def test_existing_file_needs_explicit_force(self):
        self.target.write_bytes(b'KEEP')
        with self.assertRaises(ValueError):builder.create(self.efi,self.target)
        self.assertEqual(self.target.read_bytes(),b'KEEP')
    def test_symlink_even_with_force_is_rejected(self):
        self.target.symlink_to(self.efi)
        with self.assertRaises(ValueError):builder.create(self.efi,self.target,True)
        self.assertEqual(self.efi.read_bytes(),b'INPUT')
    def test_device_target_is_rejected(self):
        with self.assertRaises(ValueError):builder.create(self.efi,'/dev/null',True)
    def test_input_cannot_be_overwritten(self):
        with self.assertRaises(ValueError):builder.create(self.efi,self.efi,True)
    def test_failed_tool_preserves_old_image_and_removes_temporary(self):
        self.target.write_bytes(b'KEEP')
        with patch.dict(os.environ,{'MKFS_FAT':'/does/not/exist'}):
            with self.assertRaises(FileNotFoundError):builder.create(self.efi,self.target,True)
        self.assertEqual(self.target.read_bytes(),b'KEEP')
        self.assertEqual(list(self.folder.glob('.basic-efi-*')),[])
    def test_racing_creator_is_not_overwritten(self):
        def fake_run(*args,**kwargs):self.target.write_bytes(b'RACER')
        with patch.object(builder.subprocess,'run',side_effect=fake_run):
            with self.assertRaises(FileExistsError):builder.create(self.efi,self.target)
        self.assertEqual(self.target.read_bytes(),b'RACER')
    def test_force_replaces_inode_without_modifying_hardlink_peer(self):
        peer=self.folder/'peer';peer.write_bytes(b'KEEP');os.link(peer,self.target)
        with patch.object(builder.subprocess,'run'):builder.create(self.efi,self.target,True)
        self.assertEqual(peer.read_bytes(),b'KEEP');self.assertEqual(self.target.stat().st_size,64*1024*1024)

if __name__=='__main__':unittest.main()
