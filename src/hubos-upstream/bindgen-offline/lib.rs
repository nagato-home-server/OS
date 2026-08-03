use std::fs;
use std::path::Path;

#[derive(Default, Clone)]
pub struct Builder;

impl Builder {
    pub fn default() -> Self {
        Self
    }

    pub fn header_contents(self, _name: &str, _contents: &str) -> Self {
        self
    }

    pub fn detect_include_paths(self, _enabled: bool) -> Self {
        self
    }

    pub fn clang_args<I, S>(self, _args: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        self
    }

    pub fn clang_arg<S: Into<String>>(self, _arg: S) -> Self {
        self
    }

    pub fn ignore_functions(self) -> Self {
        self
    }

    pub fn blocklist_item<S: Into<String>>(self, _item: S) -> Self {
        self
    }

    pub fn constified_enum_module<S: Into<String>>(self, _pattern: S) -> Self {
        self
    }

    pub fn derive_eq(self, _enabled: bool) -> Self {
        self
    }

    pub fn derive_default(self, _enabled: bool) -> Self {
        self
    }

    pub fn generate_comments(self, _enabled: bool) -> Self {
        self
    }

    pub fn use_core(self) -> Self {
        self
    }

    pub fn generate(self) -> Result<Bindings, std::io::Error> {
        Ok(Bindings {
            contents: generate_bindings(),
        })
    }
}

pub struct Bindings {
    contents: String,
}

impl Bindings {
    pub fn write_to_file<P: AsRef<Path>>(&self, path: P) -> Result<(), std::io::Error> {
        fs::write(path, &self.contents)
    }
}

fn generate_bindings() -> String {
    let contents = r#"
pub type seL4_Int8 = i8;
pub type seL4_Uint8 = u8;
pub type seL4_Int16 = i16;
pub type seL4_Uint16 = u16;
pub type seL4_Int32 = i32;
pub type seL4_Uint32 = u32;
pub type seL4_Int64 = i64;
pub type seL4_Uint64 = u64;
pub type seL4_Bool = seL4_Int8;
pub type seL4_Word = u64;
pub type seL4_CPtr = seL4_Word;
pub type seL4_NodeId = seL4_Word;
pub type seL4_PAddr = seL4_Word;
pub type seL4_Domain = seL4_Word;
pub type seL4_CNode = seL4_CPtr;
pub type seL4_IRQHandler = seL4_CPtr;
pub type seL4_IRQControl = seL4_CPtr;
pub type seL4_TCB = seL4_CPtr;
pub type seL4_Untyped = seL4_CPtr;
pub type seL4_DomainSet = seL4_CPtr;
pub type seL4_SchedContext = seL4_CPtr;
pub type seL4_SchedControl = seL4_CPtr;
pub type seL4_Time = seL4_Uint64;
pub type seL4_SlotPos = seL4_Word;

pub mod seL4_MsgLimits {
    pub type Type = u32;
    pub const seL4_MsgLengthBits: Type = 7;
    pub const seL4_MsgExtraCapBits: Type = 2;
    pub const seL4_MsgMaxLength: Type = 120;
}

pub const seL4_MsgMaxExtraCaps: usize = 3;

pub const seL4_WordBits: usize = 64;
pub const seL4_WordSizeBits: usize = 3;
pub const seL4_SlotBits: usize = 5;
pub const seL4_NotificationBits: usize = 6;
pub const seL4_ReplyBits: usize = 5;
pub const seL4_EndpointBits: usize = 4;
pub const seL4_IPCBufferSizeBits: usize = 10;
pub const seL4_TCBBits: usize = 11;
pub const seL4_PageTableEntryBits: usize = 3;
pub const seL4_PageTableIndexBits: usize = 9;
pub const seL4_PageBits: usize = 12;
pub const seL4_LargePageBits: usize = 21;
pub const seL4_HugePageBits: usize = 30;
pub const seL4_TeraPageBits: usize = 39;
pub const seL4_PageTableBits: usize = 12;
pub const seL4_VSpaceBits: usize = seL4_PageTableBits;
pub const seL4_NumASIDPoolsBits: usize = 7;
pub const seL4_ASIDPoolIndexBits: usize = 9;
pub const seL4_ASIDPoolBits: usize = 12;
pub const seL4_MinUntypedBits: usize = 4;
pub const seL4_MaxUntypedBits: usize = 38;
pub const seL4_MinSchedContextBits: usize = 7;

pub const DEBUG_MESSAGE_START: usize = 6;
pub const DEBUG_MESSAGE_MAXLEN: usize = 50;

pub mod seL4_RootCNodeCapSlots {
    pub type Type = u32;
    pub const seL4_CapNull: Type = 0;
    pub const seL4_CapInitThreadTCB: Type = 1;
    pub const seL4_CapInitThreadCNode: Type = 2;
    pub const seL4_CapInitThreadVSpace: Type = 3;
    pub const seL4_CapIRQControl: Type = 4;
    pub const seL4_CapASIDControl: Type = 5;
    pub const seL4_CapInitThreadASIDPool: Type = 6;
    pub const seL4_CapIOPortControl: Type = 7;
    pub const seL4_CapIOSpace: Type = 8;
    pub const seL4_CapBootInfoFrame: Type = 9;
    pub const seL4_CapInitThreadIPCBuffer: Type = 10;
    pub const seL4_CapDomain: Type = 11;
    pub const seL4_CapSMMUSIDControl: Type = 12;
    pub const seL4_CapSMMUCBControl: Type = 13;
    pub const seL4_CapInitThreadSC: Type = 14;
    pub const seL4_CapSMC: Type = 15;
    pub const seL4_NumInitialCaps: Type = 16;
}

pub mod seL4_BootInfoID {
    pub type Type = u32;
    pub const SEL4_BOOTINFO_HEADER_PADDING: Type = 0;
    pub const SEL4_BOOTINFO_HEADER_X86_VBE: Type = 1;
    pub const SEL4_BOOTINFO_HEADER_X86_MBMMAP: Type = 2;
    pub const SEL4_BOOTINFO_HEADER_X86_ACPI_RSDP: Type = 3;
    pub const SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER: Type = 4;
    pub const SEL4_BOOTINFO_HEADER_X86_TSC_FREQ: Type = 5;
    pub const SEL4_BOOTINFO_HEADER_FDT: Type = 6;
    pub const SEL4_BOOTINFO_HEADER_NUM: Type = 7;
}

pub mod _object {
    pub type Type = u32;
    pub const seL4_RISCV_4K_Page: Type = 8;
    pub const seL4_RISCV_Mega_Page: Type = 9;
    pub const seL4_RISCV_PageTableObject: Type = 10;
    pub const seL4_RISCV_4K: Type = 8;
    pub const seL4_RISCV_LargePageObject: Type = 9;
}

pub mod _mode_object {
    pub type Type = u32;
    pub const seL4_RISCV_Giga_Page: Type = 7;
    pub const seL4_ModeObjectTypeCount: Type = 8;
}

pub mod seL4_RISCV_VMAttributes {
    pub type Type = u32;
    pub const seL4_RISCV_Default_VMAttributes: Type = 0;
    pub const seL4_RISCV_ExecuteNever: Type = 1;
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct seL4_SlotRegion {
    pub start: seL4_SlotPos,
    pub end: seL4_SlotPos,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct seL4_UntypedDesc {
    pub paddr: seL4_Word,
    pub sizeBits: seL4_Uint8,
    pub isDevice: seL4_Uint8,
    pub padding: [u8; 6],
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct seL4_BootInfoHeader {
    pub id: seL4_Word,
    pub len: seL4_Word,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct seL4_UserContext {
    pub pc: seL4_Word,
    pub ra: seL4_Word,
    pub sp: seL4_Word,
    pub gp: seL4_Word,
    pub s0: seL4_Word,
    pub s1: seL4_Word,
    pub s2: seL4_Word,
    pub s3: seL4_Word,
    pub s4: seL4_Word,
    pub s5: seL4_Word,
    pub s6: seL4_Word,
    pub s7: seL4_Word,
    pub s8: seL4_Word,
    pub s9: seL4_Word,
    pub s10: seL4_Word,
    pub s11: seL4_Word,
    pub a0: seL4_Word,
    pub a1: seL4_Word,
    pub a2: seL4_Word,
    pub a3: seL4_Word,
    pub a4: seL4_Word,
    pub a5: seL4_Word,
    pub a6: seL4_Word,
    pub a7: seL4_Word,
    pub t0: seL4_Word,
    pub t1: seL4_Word,
    pub t2: seL4_Word,
    pub t3: seL4_Word,
    pub t4: seL4_Word,
    pub t5: seL4_Word,
    pub t6: seL4_Word,
    pub tp: seL4_Word,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct seL4_IPCBuffer {
    pub tag: seL4_MessageInfo,
    pub msg: [seL4_Word; 120],
    pub userData: seL4_Word,
    pub caps_or_badges: [seL4_Word; 3],
    pub receiveCNode: seL4_CPtr,
    pub receiveIndex: seL4_CPtr,
    pub receiveDepth: seL4_Word,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct seL4_BootInfo {
    pub extraLen: seL4_Word,
    pub nodeID: seL4_NodeId,
    pub numNodes: seL4_Word,
    pub numIOPTLevels: seL4_Word,
    pub ipcBuffer: *mut seL4_IPCBuffer,
    pub empty: seL4_SlotRegion,
    pub sharedFrames: seL4_SlotRegion,
    pub userImageFrames: seL4_SlotRegion,
    pub userImagePaging: seL4_SlotRegion,
    pub ioSpaceCaps: seL4_SlotRegion,
    pub extraBIPages: seL4_SlotRegion,
    pub initThreadCNodeSizeBits: seL4_Word,
    pub initThreadDomain: seL4_Domain,
    pub schedcontrol: seL4_SlotRegion,
    pub untyped: seL4_SlotRegion,
    pub untypedList: [seL4_UntypedDesc; 230],
}

"#;
    contents.to_owned()
}
