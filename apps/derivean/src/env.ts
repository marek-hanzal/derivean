import {withEnv} from "@use-pico/env";
import {schema}  from "@use-pico/schema";

export const {env} = withEnv({
    client:     schema(z => z.object({})),
    server:     schema((z, p) => z.object({
        DATABASE_URL:                  z.nonEmptyString,
        NODE_ENV:                      z.enum([
            "development",
            "test",
            "production",
        ]),
        NEXTAUTH_GITHUB_CLIENT_ID:     z.nonEmptyString,
        NEXTAUTH_GITHUB_CLIENT_SECRET: z.nonEmptyString,
        NEXTAUTH_URL:                  z.string,
        NEXTAUTH_SECRET:               process.env.NODE_ENV === "production"
                                           ? z.nonEmptyString
                                           : z.nonEmptyString.optional(),
    })),
    processEnv: {
        DATABASE_URL:                  process.env.DATABASE_URL,
        NEXTAUTH_GITHUB_CLIENT_ID:     process.env.NEXTAUTH_GITHUB_CLIENT_ID,
        NEXTAUTH_GITHUB_CLIENT_SECRET: process.env.NEXTAUTH_GITHUB_CLIENT_SECRET,
        NEXTAUTH_SECRET:               process.env.NEXTAUTH_SECRET,
        NEXTAUTH_URL:                  process.env.NEXTAUTH_URL,
        NODE_ENV:                      process.env.NODE_ENV,
    },
});
