import {
    KingdomLayout,
    KingdomMenu,
    withKingdomRepository
}                               from "@derivean/kingdom";
import {container}              from "@derivean/server";
import {t}                      from "@use-pico/i18n";
import {
    Divider,
    Group,
    Title
}                               from "@use-pico/ui";
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
        homeUrl={`/kingdom/${kingdom.id}`}
        logo={logo}
        center={<Group>
            <Divider orientation={"vertical"}/>
            <Title c={"dimmed"} order={3}>{t({values: kingdom})`Selected kingdom`}</Title>
            <Divider orientation={"vertical"}/>
            <KingdomMenu kingdomId={kingdom.id}/>
        </Group>}
    >
        {children}
    </KingdomLayout>;
}
