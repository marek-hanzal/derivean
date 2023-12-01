import {KingdomMenu}            from "@derivean/game";
import {
    KingdomLayout,
    withKingdomRepository
}                               from "@derivean/kingdom";
import {container}              from "@derivean/server";
import {type PropsWithChildren} from "react";
import logo                     from "../../../../public/assets/logo/logo.svg";

export namespace Layout {
    export type Props = PropsWithChildren<{
        params: {
            kingdomId: string;
        };
    }>;
}

export default async function Layout(
    {
        children,
        params: {kingdomId},
    }: Layout.Props
) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <KingdomLayout
        header={{
            home:   `/kingdom/${kingdom.id}`,
            logo,
            center: <div
                        className={"flex flex-row items-center"}
                    >
                        {/*<ButtonLink*/}
                        {/*    href={"/game"}*/}
                        {/*    size={"sm"}*/}
                        {/*    cn={[*/}
                        {/*        "ml-4",*/}
                        {/*        "mr-1",*/}
                        {/*    ]}*/}
                        {/*>*/}
                        {/*    <Icon icon={"i-tabler:arrow-left"}/>*/}
                        {/*</ButtonLink>*/}
                        {/*<Title*/}
                        {/*    c={"text-zinc-600"}*/}
                        {/*    order={4}*/}
                        {/*    cn={[*/}
                        {/*        "text-md",*/}
                        {/*        "font-bold",*/}
                        {/*    ]}*/}
                        {/*>*/}
                        {/*    {tv(kingdom)`Selected kingdom`}*/}
                        {/*</Title>*/}
                        <KingdomMenu
                            kingdomId={kingdom.id}
                            cn={[
                                "ml-4",
                            ]}
                        />
                    </div>,
        }}
    >
        {children}
    </KingdomLayout>;
}
