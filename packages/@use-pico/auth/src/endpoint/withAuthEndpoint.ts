import {type IContainer}        from "@use-pico/container";
import {withLogger}             from "@use-pico/logger";
import NextAuthShit, {
    type AuthOptions,
    type Session
}                               from "next-auth";
import {type Provider}          from "next-auth/providers";
import {useRegistrationService} from "../use/useRegistrationService";
import {useUserTokenService}    from "../use/useUserTokenService";

export namespace withAuthEndpoint {
    export interface Props {
        options?: Partial<AuthOptions>;
        providers: (Provider | null | false | undefined)[];
        container: IContainer.Type;
    }
}

export const withAuthEndpoint = (
    {
        options,
        providers,
        container,
    }: withAuthEndpoint.Props
) => {
    const registrationService = useRegistrationService(container);
    const userTokenService = useUserTokenService(container);
    const logger = withLogger("auth");
    /**
     * For whatever reason, types are not what you really get, so the hack must be used.
     */
    const NextAuth = (NextAuthShit as any).default as typeof NextAuthShit;

    return NextAuth({
        theme:     {
            logo:        "/assets/logo/logo.svg",
            brandColor:  "#1890ff",
            colorScheme: "light",
        },
        events:    {
            signIn:  ({user}) => {
                logger.debug("User sign-in", {label: {userId: user.id}});
            },
            signOut: ({token: {sub}}) => {
                logger.debug("User sign-out", {label: {userId: sub}});
            },
        },
        session:   {
            strategy: "jwt",
        },
        providers: providers.filter(Boolean),
        callbacks: {
            jwt:     async token => {
                try {
                    await registrationService.handle(token);
                    return await userTokenService.token(token.token);
                } catch (e) {
                    if (e instanceof Error) {
                        logger.error(e.message);
                        logger.error(e.stack);
                    }
                    throw e;
                }
            },
            session: async (
                {
                    session,
                    token
                }
            ) => {
                const $session: any = {...session};
                if ($session && token?.sub) {
                    $session.user = {
                        userId: token.sub,
                        tokens: token.tokens,
                        ...session.user,
                    };
                }
                return $session as Session;
            },
        },
        ...options,
    });
};
