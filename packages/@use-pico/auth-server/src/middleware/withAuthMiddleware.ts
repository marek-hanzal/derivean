import {withAuth}     from "next-auth/middleware";
import {NextResponse} from "next/server";

export namespace withAuthMiddleware {
    export interface Props {
        routes?: Route[];
    }

    export interface Route {
        path: string;
        target: string;
        auth: boolean;
        tokens?: string[];
    }
}

export const withAuthMiddleware = (
    {
        routes = [],
    }: withAuthMiddleware.Props
) => {
    return withAuth(
        request => {
            const token = request.nextauth.token;
            for (const {
                path,
                target,
                auth
            } of routes) {
                if (request.nextUrl.pathname.includes(path)) {
                    console.log(`Redirect check for [${request.nextUrl.pathname}] on [${path}]`, token);
                    if ((token && !auth) || (!token && auth)) {
                        return NextResponse.redirect(
                            new URL(
                                target,
                                request.url
                            )
                        );
                    }
                }
            }
            return NextResponse.next();
        },
        {
            callbacks: {
                authorized({
                               req,
                               token
                           }) {
                    for (const {
                        path,
                        auth
                    } of routes) {
                        if (req.nextUrl.pathname.includes(path)) {
                            console.log(`Auth check for [${req.nextUrl.pathname}] on [${path}]`, token);
                            if (!token && auth) {
                                return false;
                            }
                        }
                    }
                    return true;
                },
            },
        }
    );
};
