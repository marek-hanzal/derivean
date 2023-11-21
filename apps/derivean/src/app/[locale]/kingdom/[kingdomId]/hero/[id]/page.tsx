import {withHeroRepository}    from "@derivean/hero";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {HeroIcon}              from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
            id: string;
        };
    }
}

export default async function List(
    {
        params: {
                    kingdomId,
                    id
                }
    }: List.Props
) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);
    const hero = await withHeroRepository.use(container).getOrThrow(id);

    return <Page
        icon={<HeroIcon/>}
        text={{
            header: t({values: hero})`Kingdom hero detail (label)`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href:  "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  {
                        href:  "/kingdom/[kingdomId]/hero/list",
                        query: {kingdomId: kingdom.id},
                    },
                    label: t()`Kingdom hero list (label)`,
                    icon:  <HeroIcon/>,
                },
            ]}
        />}
    >
        hero detail
    </Page>;
}
