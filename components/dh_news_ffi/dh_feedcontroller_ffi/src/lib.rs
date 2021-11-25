pub fn hello_world() {
    println!("Hello World");
}

#[no_mangle]
pub extern "C" fn rust_function() {
    println!("Hello World");
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }
}
