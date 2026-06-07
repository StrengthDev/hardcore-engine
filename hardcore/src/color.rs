use crate::Error;

#[derive(Debug, Copy, Clone)]
pub struct Color {
    // Internal values are in the unsigned normalized format (all channel values are in the range [0.0, 1.0])
    r: f32,
    g: f32,
    b: f32,
    a: f32,
}

impl Color {
    pub fn from_unorm(r: f32, g: f32, b: f32, a: f32) -> Result<Color, Error> {
        if (0.0..=1.0).contains(&r)
            && (0.0..=1.0).contains(&g)
            && (0.0..=1.0).contains(&b)
            && (0.0..=1.0).contains(&a)
        {
            Ok(Color { r, g, b, a })
        } else {
            Err(Error::InvalidParams(
                "all color channel values must be in the range [0.0, 1.0]".to_string(),
            ))
        }
    }

    pub fn from_snorm(r: f32, g: f32, b: f32, a: f32) -> Result<Color, Error> {
        if (-1.0..=1.0).contains(&r)
            && (-1.0..=1.0).contains(&g)
            && (-1.0..=1.0).contains(&b)
            && (-1.0..=1.0).contains(&a)
        {
            Ok(Color {
                r: r * 0.5 + 0.5,
                g: g * 0.5 + 0.5,
                b: b * 0.5 + 0.5,
                a: a * 0.5 + 0.5,
            })
        } else {
            Err(Error::InvalidParams(
                "all color channel values must be in the range [-1.0, 1.0]".to_string(),
            ))
        }
    }

    pub const fn from_u8(r: u8, g: u8, b: u8, a: u8) -> Color {
        let max = u8::MAX as f32 + 1.0;
        let half_step = 0.5 / max;

        Color {
            r: r as f32 / max + half_step,
            g: g as f32 / max + half_step,
            b: b as f32 / max + half_step,
            a: a as f32 / max + half_step,
        }
    }

    pub const fn from_u32(r: u32, g: u32, b: u32, a: u32) -> Color {
        let max = u32::MAX as f32 + 1.0;
        let half_step = 0.5 / max;

        Color {
            r: r as f32 / max + half_step,
            g: g as f32 / max + half_step,
            b: b as f32 / max + half_step,
            a: a as f32 / max + half_step,
        }
    }

    pub fn from_hex(code: impl AsRef<str>) -> Result<Color, Error> {
        let hex = code.as_ref();
        if hex.len() != 9 {
            return Err(Error::InvalidParams(
                "hex code has incorrect size".to_string(),
            ));
        }

        let hex = if let Some(stripped) = hex.strip_prefix('#') {
            stripped
        } else {
            return Err(Error::InvalidParams(format!(
                "hex code has incorrect prefix \"{}\"",
                &hex[0..1]
            )));
        };

        let (channels0, channels1) = hex.split_at(4);
        let (r, g) = channels0.split_at(2);
        let (b, a) = channels1.split_at(2);

        let r = if let Ok(parsed) = u8::from_str_radix(r, 16) {
            parsed
        } else {
            return Err(Error::InvalidParams(format!(
                "failed to parse red value \"{r}\""
            )));
        };
        let g = if let Ok(parsed) = u8::from_str_radix(g, 16) {
            parsed
        } else {
            return Err(Error::InvalidParams(format!(
                "failed to parse red value \"{g}\""
            )));
        };
        let b = if let Ok(parsed) = u8::from_str_radix(b, 16) {
            parsed
        } else {
            return Err(Error::InvalidParams(format!(
                "failed to parse red value \"{b}\""
            )));
        };
        let a = if let Ok(parsed) = u8::from_str_radix(a, 16) {
            parsed
        } else {
            return Err(Error::InvalidParams(format!(
                "failed to parse red value \"{a}\""
            )));
        };

        Ok(Self::from_u8(r, g, b, a))
    }

    pub fn as_unorm(&self) -> (f32, f32, f32, f32) {
        (self.r, self.g, self.b, self.a)
    }

    pub fn as_snorm(&self) -> (f32, f32, f32, f32) {
        (
            (self.r - 0.5) * 2.0,
            (self.g - 0.5) * 2.0,
            (self.b - 0.5) * 2.0,
            (self.a - 0.5) * 2.0,
        )
    }

    pub fn as_rgba_u8(&self) -> (u8, u8, u8, u8) {
        let max = u8::MAX as f32 + 1.0;
        let half_step = 0.5 / max;

        (
            ((self.r - half_step) * max) as u8,
            ((self.g - half_step) * max) as u8,
            ((self.b - half_step) * max) as u8,
            ((self.a - half_step) * max) as u8,
        )
    }

    pub fn as_rgba_u32(&self) -> (u32, u32, u32, u32) {
        let max = u32::MAX as f32 + 1.0;
        let half_step = 0.5 / max;

        (
            ((self.r - half_step) * max) as u32,
            ((self.g - half_step) * max) as u32,
            ((self.b - half_step) * max) as u32,
            ((self.a - half_step) * max) as u32,
        )
    }

    pub fn hex(&self) -> String {
        let (r, g, b, a) = self.as_rgba_u8();
        format!("#{r:0>2x}{g:0>2x}{b:0>2x}{a:0>2x}")
    }

    pub fn inverse(&self, keep_alpha: bool) -> Color {
        let a = if keep_alpha { self.a } else { 1.0 - self.a };

        Color {
            r: 1.0 - self.r,
            g: 1.0 - self.g,
            b: 1.0 - self.b,
            a,
        }
    }

    pub fn mix(a: &Color, b: &Color, interpolation: f32) -> Color {
        let interpolation = interpolation.clamp(0.0, 1.0);

        Color {
            r: a.r + (b.r - a.r) * interpolation,
            g: a.g + (b.g - a.g) * interpolation,
            b: a.b + (b.b - a.b) * interpolation,
            a: a.a + (b.a - a.a) * interpolation,
        }
    }

    // TODO fancier mix methods
}

impl std::ops::Mul for Color {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Color {
            r: self.r * rhs.r,
            g: self.g * rhs.g,
            b: self.b * rhs.b,
            a: self.a * rhs.a,
        }
    }
}

impl std::ops::MulAssign for Color {
    fn mul_assign(&mut self, rhs: Self) {
        self.r *= rhs.r;
        self.g *= rhs.g;
        self.b *= rhs.b;
        self.a *= rhs.a;
    }
}

impl From<Color> for hardcore_sys::Color {
    fn from(value: Color) -> Self {
        hardcore_sys::Color {
            r: value.r,
            g: value.g,
            b: value.b,
            a: value.a,
        }
    }
}

pub static WHITE: Color = Color {
    r: 1.0,
    g: 1.0,
    b: 1.0,
    a: 1.0,
};

pub static BLACK: Color = Color {
    r: 0.0,
    g: 0.0,
    b: 0.0,
    a: 0.0,
};

mod tests {
    // TODO tests
}
