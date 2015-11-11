#![feature(asm)]

extern crate getopts;

use std::env;
use std::process;
use std::ffi::{CString,CStr};
use getopts::Options;

const VERSION: &'static str = env!("CARGO_PKG_VERSION");

// System call numbers
#[cfg(target_arch = "x86_64")]
const SYS_RENAMEAT2: i32 = 316;
#[cfg(target_arch = "x86")]
const SYS_RENAMEAT2: i32 = 353;

const RENAME_EXCHANGE:  u32 = (1 << 1);

const AT_FDCWD: i32 = -100;

//XXX We should consider writing a static library in C to
//avoid the inline assembly below.
#[cfg(target_arch = "x86_64")]
fn sys_renameat2(olddirfd: i32, oldpath: &CStr, newdirfd: i32, newpath: &CStr, flags: u32) -> i32 {
    let ret: i32;

    // See: http://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-on-x86-64
    unsafe {
        asm!("syscall"
             : "={rax}"(ret)
             : "{rax}"(SYS_RENAMEAT2), 
               "{rdi}"(olddirfd), 
               "{rsi}"(oldpath.as_ptr()),
               "{rdx}"(newdirfd),
               "{r10}"(newpath.as_ptr()),
               "{r8}" (flags)
             : "memory", "cc", "{rcx}", "{r11}"
             : "volatile"
            );
    }

    ret
}


pub fn renameat2(oldpath: &str, newpath: &str, flags: u32) -> Result<(),String> {
    // TODO Do some checking of the paths here

    let c_oldpath = CString::new(oldpath).unwrap();
    let c_newpath = CString::new(newpath).unwrap();

    let mut ret = sys_renameat2(AT_FDCWD, &c_oldpath, AT_FDCWD, &c_newpath, flags);

    if ret <= -1 && ret >= -4095 {
        ret = ret.abs();
        // TODO Use libc::strerror_r ?
        return Err(format!("errno: {}", ret))
    } else if ret != 0 {
        // Something is off. We should only have a return value from
        // a system call thats 0 through -4095
        panic!("invalid return from syscall: ax={}", ret);
    }

    Ok( () )
}

fn main() {
    let mut verbose = false;
    let args: Vec<String> = env::args().collect();
    let mut opts = Options::new();

    opts.optflag("V", "version", "print version");
    opts.optflag("h", "help", "print this help menu");
    opts.optflag("v", "verbose", "verbose output");
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => { m }
        Err(f) => { panic!(f.to_string()) }
    };

    if matches.opt_present("h") {
        // TODO add details about options
        println!("Usage: {} [options] oldfile newfile", &args[0]);
        return;
    }
    if matches.opt_present("V") {
        println!("xchg v{}", VERSION);
        return;
    }

    let files = &matches.free;
    if files.len() > 2 {
        println!("Too many files provided");
        process::exit(1)
    } else if files.len() < 2 {
        println!("Must provide new and old file paths");
        process::exit(1)
    }

    if matches.opt_present("v") {
        verbose = true;
    }


    match renameat2(&files[0], &files[1], RENAME_EXCHANGE) {
        Ok(_) => {
            if verbose {
                println!("`{}` <-> `{}`", files[0], files[1])
            }
        }
        Err(e) => {
            println!("{}", e);
            process::exit(1)
        }
    }
}
