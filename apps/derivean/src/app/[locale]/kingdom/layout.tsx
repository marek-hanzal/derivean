import {KingdomMenu}            from "@derivean/game";
import {
    KingdomLayout,
    withKingdomRepository
}                               from "@derivean/kingdom";
import {container}              from "@derivean/server";
import {IconArrowLeft}          from "@tabler/icons-react";
import {
    ButtonLink,
    Divider,
    Group,
    Title
}                               from "@use-pico/client";
import {t}                      from "@use-pico/translator";
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
        home={`/kingdom/${kingdom.id}`}
        logo={logo}
        center={<Group>
            <Divider orientation={"vertical"}/>
            <Group gap={"xs"}>
                <ButtonLink
                    href={"/game"}
                    label={<IconArrowLeft/>}
                />
                <Title c={"dimmed"} order={4}>{t({values: kingdom})`Selected kingdom`}</Title>
            </Group>
            <Divider orientation={"vertical"}/>
            <KingdomMenu kingdomId={kingdom.id}/>
        </Group>}
    >
        {children}
    </KingdomLayout>;
}
