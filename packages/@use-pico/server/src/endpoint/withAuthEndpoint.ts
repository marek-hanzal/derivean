import {KyselyAdapter}              from "@auth/kysely-adapter";
import NextAuth, {type AuthOptions} from "next-auth";
import {type Provider}              from "next-auth/providers";
import {type Connection}            from "../api/Connection";
import {type IContainer}            from "../api/IContainer";
import {withConnection}             from "../container/withConnection";
import {withUserTokenService}       from "../container/withUserTokenService";
import {withLogger}                 from "../utils/withLogger";

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
    const userTokenService = withUserTokenService.use(container);
    const logger = withLogger("auth");

    return NextAuth({
        theme:     {
            logo:        "/assets/logo/logo.svg",
            brandColor:  "#1890ff",
            colorScheme: "light",
        },
        session:   {
            strategy: "jwt",
        },
        adapter:   KyselyAdapter(withConnection.use(container) as Connection<any>),
        providers: providers.filter(Boolean),
        callbacks: {
            jwt: async ({token}) => {
                try {
                    return await userTokenService.token(token);
                } catch (e) {
                    if (e instanceof Error) {
                        logger.error(e.message);
                        logger.error(e.stack);
                    }
                    throw e;
                }
            },
        },
        ...options,
    });
};
