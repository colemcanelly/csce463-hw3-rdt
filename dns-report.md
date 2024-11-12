# Report: Homework 2 - DNS Lookup

Cole McAnelly  
CS 463

1. **[Case 1](#case-1)**
    1. [`random0.irl`](#random0irl)
    2. [`random3.irl`](#random3irl)
    3. [`random5.irl`](#random5irl)
    4. [`random6.irl`](#random6irl)
2. **[Case 2](#case-2-random1irl)**
3. **[Case 3](#case-3-random7irl)**
4. **[Case 4](#case-4-random4irl)**
5. **[Extra Credit](#extra-credit)**

<!-- ####################################################################################################### -->
## Case 1

### `random0.irl`

```text
Lookup  : random0.irl
Query   : random0.irl, type 1, TXID 0x0000
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 178ms with 82 bytes
  TXID 0x0000, flags 0x8400, questions 1, answers 2, authority 0, additional 0
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random0.irl type 1 class 1
  ------------ [answers] ------------
  ++ invalid record: jump into fixed DNS header
```

This error is caused what there is a jump offset that is less than the size of the `FixedDNSHeader`, which attempts to overwrite crucial data. This can be caught with a simple bounds check before jumping.  

### `random3.irl`

```txt
Lookup  : random3.irl
Query   : random3.irl, type 1, TXID 0x0003
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... 
  ++ invalid reply: packet smaller than fixed DNS header
```

This error occurs when the DNS server attempts to return a packet that is smaller than the mandatory fixed header size. This can be caught immediately after the receive functionality.

### `random5.irl`

```txt
Lookup  : random5.irl
Query   : random5.irl, type 1, TXID 0x0007
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 17ms with 71 bytes
  TXID 0x0007, flags 0x8400, questions 1, answers 2, authority 0, additional 0
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random5.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 1.1.1.1 TTL = 30
  ++ invalid record: jump beyond packet boundary
```

This error occurs when the jump offset is outside of the packet boundary. This can be checked with a simple bounds check before jumping.

### `random6.irl`

```txt
Lookup  : random6.irl
Query   : random6.irl, type 1, TXID 0x0008
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 21ms with 59 bytes
  TXID 0x0008, flags 0x8400, questions 1, answers 2, authority 0, additional 0
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random6.irl type 1 class 1
  ------------ [answers] ------------
  ++ invalid record: jump loop
```

This error occurs when there is a number of jumps that cause a high number of them to occur. We can prevent this by checking that the number of jumps doesn't get higher than the maximum amount of jumps that could be stored in a packet.


<!-- ####################################################################################################### -->
## Case 2 (`random1.irl`)

```txt
Lookup  : random1.irl
Query   : random1.irl, type 1, TXID 0x0001
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 42ms with 468 bytes
  TXID 0x0001, flags 0x8600, questions 1, answers 1, authority 0, additional 65535
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random1.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 1.1.1.1 TTL = 30
  ------------ [additional] ---------
  ++ invalid section: not enough records
```

This errors is caused by the amount of actual records is greater than the total number of stated records. This can be checked by counting the number of responses recieved, and comparing it against the stated number of responses in the fixed DNS header.



<!-- ####################################################################################################### -->
## Case 3 (`random7.irl`)

```txt
Lookup  : random7.irl
Query   : random7.irl, type 1, TXID 0x0009
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 13ms with 42 bytes
  TXID 0x0009, flags 0x8400, questions 1, answers 2, authority 0, additional 0
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random7.irl type 1 class 1
  ------------ [answers] ------------
  ++ invalid record: truncated jump offset
```

This error can happen if the packet ends right in between the jump offset bytes, so the program can know that it should jump, it just doesn't have the lower 8 bits of the offset address. We can check this by doing a bounds check on the next byte before we use it for the lower half of the offset address. 

<!-- ####################################################################################################### -->
## Case 4 (`random4.irl`)

```txt
Lookup  : random4.irl
Query   : random4.irl, type 1, TXID 0x000A
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 33ms with 324 bytes
  TXID 0x000A, flags 0x8400, questions 1, answers 1, authority 0, additional 11
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random4.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 1.1.1.1 TTL = 30
  ------------ [additional] ---------
        Episode.IV A 2.2.2.2 TTL = 30
        A.NEW.HOPE A 2.2.2.2 TTL = 30
        It.is.a.period.of.civil.war A 2.2.2.2 TTL = 30
        Rebel.spaceships A 2.2.2.2 TTL = 30
        striking.from.a.hidden.base A 2.2.2.2 TTL = 30
        have.won.their.first.victory A 2.2.2.2 TTL = 30
        against.the.evil.Galactic.Empire A 2.2.2.2 TTL = 30
  ++ invalid record: truncated RR answer header
```
This error occurs when the Answer header has been corrupted in some way, either it was cut off by the packet ending, or other data has malformed it. We can check it be doing a bounds check on the current position *plus* the size of the Answer Header. By doing this, we can ensure that all of the data in the answer header is left unmodified.


```txt
Lookup  : random4.irl
Query   : random4.irl, type 1, TXID 0x0000
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 62ms with 54 bytes
  TXID 0x0000, flags 0x8400, questions 1, answers 1, authority 0, additional 11
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random4.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 0.1.1.1 TTL = 30
  ------------ [additional] ---------
  ++ invalid section: not enough records
```

This errors is caused by the amount of actual records is greater than the total number of stated records. This can be checked by counting the number of responses recieved, and comparing it against the stated number of responses in the fixed DNS header.


```txt
Lookup  : random4.irl
Query   : random4.irl, type 1, TXID 0x0004
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 19ms with 144 bytes
  TXID 0x0004, flags 0x8400, questions 1, answers 1, authority 0, additional 11
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random4.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 1.1.1.1 TTL = 30
  ------------ [additional] ---------
        Episode.IV A 2.2.2.2 TTL = 30
        A.NEW.HOPE A 2.2.2.2 TTL = 30
  ++ invalid record: truncated name
```

This error happens when the name in the packet is imcomplete, most likely because of abrupt packing ending. We can prevent this by doing bounds checks on the iterater as we go through the name, doing a bounds check each time.

<!-- ####################################################################################################### -->
## Extra Credit 
I queried this hostname repeatedly so that I could get a good capture of as many DNS requests as possible in Wireshark, which helped me to see the patterns that were there. 

```txt
Lookup  : random8.irl
Query   : random8.irl, type 1, TXID 0x0001
Server  : 128.194.135.82
********************************
Attempt 0 with 29 bytes... response in 4ms with 468 bytes
  TXID 0x0001, flags 0x8400, questions 1, answers 1, authority 0, additional 11
  succeeded with Rcode = 0
  ------------ [questions] ----------
        random8.irl type 1 class 1
  ------------ [answers] ------------
        random.irl A 1.1.1.1 TTL = 30
  ------------ [additional] ---------
        Episode.IV A 2.2.2.2 TTL = 30
        A.NEW.HOPE A 2.2.2.2 TTL = 30
        It.is.a.period.of.civil.war A 2.2.2.2 TTL = 30
        Rebel.spaceships A 2.2.2.2 TTL = 30
        striking.from.a.hidden.base A 2.2.2.2 TTL = 30
        have.won.their.first.victory A 2.2.2.2 TTL = 30
        against.the.evil.Galactic.Empire A 2.2.2.2 TTL = 30
        During.the.battle A 2.2.2.2 TTL = 30
        Rebel.spies.managed A 108.111.108.108 TTL = 1819045740
  ++ invalid record: RR value length stretches the answer beyond packet
```

![image](./extra_credit_wireshark_2.png)
![image](./extra_credit_wireshark.png)

This endpoint on the server takes the default response (11 records in the `additional` name entries that are from the intro sequence to Star Wars: A New Hope), and randomly chooses a places among the records. It then enters 2 null bytes before "padding" the rest of the packet with "lol" until the length of the packet is 510 bytes in length. This would be pretty easy with a random number generator and knowing the current legnth of the packet.